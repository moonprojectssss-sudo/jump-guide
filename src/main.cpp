#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/ui/Notification.hpp>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

// ===== Data.hpp =====
using namespace geode::prelude;

namespace jg {

struct Span { float x0, x1; bool ok; };

// One segment on the bar (a spike cluster = tap, a marker run = hold)
struct Evt {
    float x0 = 0.f, x1 = 0.f;
    bool hold = false;
    // runtime state, never saved
    float cursor = 0.f;
    bool hit = false, done = false;
    std::vector<Span> spans;
};

inline double fnum(char const* k) { return Mod::get()->getSettingValue<double>(k); }
inline int inum(char const* k) { return (int)Mod::get()->getSettingValue<int64_t>(k); }
inline bool flag(char const* k) { return Mod::get()->getSettingValue<bool>(k); }
inline std::string text(char const* k) { return Mod::get()->getSettingValue<std::string>(k); }

// Player speed value -> world units per second
inline float unitsPerSec(float s) {
    if (s < 0.8f) return 251.16f;
    if (s < 1.0f) return 311.58f;
    if (s < 1.2f) return 387.42f;
    if (s < 1.45f) return 468.0f;
    return 576.0f;
}

struct Obj { int id = 0, c1 = 0, c2 = 0; float x = 0.f, rot = 0.f; bool flipY = false; };

// Parses "key,value,key,value" for one object
inline bool readObj(char const* p, char const* end, Obj& o) {
    int key = 0;
    bool isKey = true;
    while (p < end) {
        if (isKey) key = (int)std::strtol(p, nullptr, 10);
        else switch (key) {
            case 1:  o.id = (int)std::strtol(p, nullptr, 10); break;
            case 2:  o.x = std::strtof(p, nullptr); break;
            case 5:  o.flipY = std::strtol(p, nullptr, 10) != 0; break;
            case 6:  o.rot = std::strtof(p, nullptr); break;
            case 21: o.c1 = (int)std::strtol(p, nullptr, 10); break;
            case 22: o.c2 = (int)std::strtol(p, nullptr, 10); break;
            default: break;
        }
        auto c = (char const*)std::memchr(p, ',', end - p);
        if (!c) break;
        p = c + 1;
        isKey = !isKey;
    }
    return o.id != 0;
}

inline std::vector<int> parseIds(std::string const& s) {
    std::vector<int> v;
    char const* p = s.c_str();
    while (*p) {
        char* n = nullptr;
        long id = std::strtol(p, &n, 10);
        if (n == p) { ++p; continue; }
        v.push_back((int)id);
        p = n;
    }
    return v;
}

inline void mergeInto(std::vector<float> xs, float gap, bool hold, std::vector<Evt>& out) {
    if (xs.empty()) return;
    std::sort(xs.begin(), xs.end());
    float a = xs[0], b = xs[0];
    auto push = [&] {
        Evt e;
        e.hold = hold;
        e.x0 = a - 15.f;
        e.x1 = b + 15.f;
        e.cursor = e.x0;
        out.push_back(e);
    };
    for (size_t i = 1; i < xs.size(); ++i) {
        if (xs[i] - b <= gap) b = xs[i];
        else { push(); a = b = xs[i]; }
    }
    push();
}

// Turns an editor clipboard string into sorted segments
inline std::vector<Evt> buildEvents(std::string const& s) {
    auto ids = parseIds(text("danger-ids"));
    int channel = inum("hold-channel");
    bool upright = flag("upright-only");
    float gap = (float)fnum("merge-gap") * 30.f;
    std::vector<float> spikes, holds;

    char const* p = s.data();
    char const* end = p + s.size();
    while (p < end) {
        auto semi = (char const*)std::memchr(p, ';', end - p);
        char const* e = semi ? semi : end;
        Obj o;
        if (readObj(p, e, o)) {
            if (channel > 0 && (o.c1 == channel || o.c2 == channel)) {
                holds.push_back(o.x);
            } else if (std::find(ids.begin(), ids.end(), o.id) != ids.end()) {
                float r = std::fmod(o.rot, 360.f);
                if (r < 0.f) r += 360.f;
                bool up = !o.flipY && (r < 45.f || r > 315.f);
                if (!upright || up) spikes.push_back(o.x);
            }
        }
        if (!semi) break;
        p = semi + 1;
    }
    std::vector<Evt> out;
    mergeInto(spikes, gap, false, out);
    mergeInto(holds, 31.f, true, out);
    std::sort(out.begin(), out.end(), [](Evt const& a, Evt const& b) { return a.x0 < b.x0; });
    return out;
}

inline std::string pack(std::vector<Evt> const& v) {
    std::string s;
    for (auto& e : v) s += fmt::format("{:.1f},{:.1f},{};", e.x0, e.x1, e.hold ? 1 : 0);
    return s;
}

inline std::vector<Evt> unpack(std::string const& s) {
    std::vector<Evt> v;
    char const* p = s.c_str();
    while (*p) {
        char* n = nullptr;
        Evt e;
        e.x0 = std::strtof(p, &n);
        if (*n != ',') break;
        p = n + 1;
        e.x1 = std::strtof(p, &n);
        if (*n != ',') break;
        p = n + 1;
        e.hold = std::strtol(p, &n, 10) != 0;
        if (*n != ';') break;
        p = n + 1;
        e.cursor = e.x0;
        v.push_back(std::move(e));
    }
    return v;
}

inline std::string keyFor(std::string const& name) { return "jg-data:" + name; }

inline void store(std::string const& name, std::vector<Evt> const& v) {
    Mod::get()->setSavedValue<std::string>(keyFor(name), pack(v));
    (void)Mod::get()->saveData();
}

inline std::vector<Evt> load(std::string const& name) {
    return unpack(Mod::get()->getSavedValue<std::string>(keyFor(name), ""));
}

} // namespace jg

// ===== Draw.hpp =====
using namespace geode::prelude;

namespace jg {

inline ccColor4F rgba(float r, float g, float b, float a = 1.f) {
    return ccColor4F{r, g, b, a};
}

inline ccColor4F accentFor(std::string const& name) {
    if (name == "Magenta") return rgba(1.00f, 0.30f, 0.85f);
    if (name == "Lime")    return rgba(0.55f, 1.00f, 0.25f);
    if (name == "Amber")   return rgba(1.00f, 0.72f, 0.15f);
    if (name == "Violet")  return rgba(0.65f, 0.45f, 1.00f);
    return rgba(0.15f, 0.90f, 1.00f); // Cyan
}

// Rounded rectangle from (x0,y0) to (x1,y1), optional border
inline void roundRect(CCDrawNode* d, float x0, float y0, float x1, float y1, float r,
                      ccColor4F fill, float bw = 0.f,
                      ccColor4F border = ccColor4F{0, 0, 0, 0}) {
    if (x1 <= x0 || y1 <= y0) return;
    r = std::max(0.01f, std::min({r, (x1 - x0) * 0.5f, (y1 - y0) * 0.5f}));
    constexpr int N = 4;
    float cx[4] = {x1 - r, x0 + r, x0 + r, x1 - r};
    float cy[4] = {y1 - r, y1 - r, y0 + r, y0 + r};
    std::vector<CCPoint> pts;
    pts.reserve(4 * (N + 1));
    for (int c = 0; c < 4; ++c) {
        for (int i = 0; i <= N; ++i) {
            float a = (c * 90.f + 90.f * i / N) * 0.0174533f;
            pts.push_back(CCPoint(cx[c] + r * std::cos(a), cy[c] + r * std::sin(a)));
        }
    }
    d->drawPolygon(pts.data(), (unsigned int)pts.size(), fill, bw, border);
}

inline void tri(CCDrawNode* d, CCPoint a, CCPoint b, CCPoint c, ccColor4F col) {
    CCPoint v[3] = {a, b, c};
    d->drawPolygon(v, 3, col, 0.f, ccColor4F{0, 0, 0, 0});
}

} // namespace jg

// ===== TimingBar.hpp =====
class TimingBar : public CCNode {
public:
    static TimingBar* create();
    static TimingBar* get() { return s_inst; }

    void loadFor(std::string const& levelName);
    void onButton(bool down);

    ~TimingBar() override {
        if (s_inst == this) s_inst = nullptr;
    }

protected:
    bool init() override;
    void update(float dt) override;

    // logic (TimingBar.cpp)
    void tickLogic(float lineX);
    void rewind(float lineX);
    void refreshLabel();

    // drawing (TimingDraw.cpp)
    void drawStatic();
    void drawFrame(float lineX, float dt);
    void drawEvent(jg::Evt const& e, float ya, float yb);
    void drawLine(float dt);
    void glowRect(float x0, float y0, float x1, float y1, ccColor4F c);

    ccColor4F col(ccColor4F c, float a = 1.f) const {
        return ccColor4F{c.r, c.g, c.b, c.a * a * m_alpha};
    }
    float yOf(float x) const { return m_yLine + (x - m_lineX) * m_pxU; }

    static inline TimingBar* s_inst = nullptr;

    CCDrawNode* m_bg = nullptr;
    CCDrawNode* m_dyn = nullptr;
    CCDrawNode* m_glow = nullptr;
    CCLabelBMFont* m_label = nullptr;
    std::vector<jg::Evt> m_events;
    ccColor4F m_accent{};

    float m_w = 0.f, m_h = 0.f, m_pad = 8.f, m_alpha = 1.f;
    float m_yBot = 10.f, m_yLine = 0.f, m_yTop = 0.f, m_pxU = 1.f;
    float m_viewUnits = 420.f, m_offsetSec = 0.09f, m_preSec = 0.12f;
    float m_lineX = 0.f, m_lastPx = 0.f, m_speed = 311.58f;
    float m_flash = 0.f, m_time = 0.f;
    bool m_down = false, m_pressed = false, m_grid = true, m_glowOn = true;
    int m_streak = 0, m_shown = -99;
    size_t m_first = 0;
};

// ===== TimingBar.cpp =====
TimingBar* TimingBar::create() {
    auto ret = new TimingBar();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool TimingBar::init() {
    if (!CCNode::init()) return false;
    auto win = CCDirector::get()->getWinSize();

    m_w = (float)jg::inum("bar-width");
    m_h = win.height * (float)jg::fnum("bar-height") / 100.f;
    m_alpha = (float)jg::fnum("opacity") / 100.f;
    m_viewUnits = (float)jg::fnum("view-distance") * 30.f;
    m_offsetSec = jg::inum("press-offset") / 1000.f;
    m_grid = jg::flag("scroll-grid");
    m_glowOn = jg::flag("glow");
    m_accent = jg::accentFor(jg::text("accent"));

    m_yLine = m_h * 0.22f;
    m_yTop = m_h - 30.f;
    m_pxU = (m_yTop - m_yLine) / m_viewUnits;

    float margin = (float)jg::inum("margin");
    bool left = jg::text("side") == "Left";
    this->setContentSize({m_w, m_h});
    this->setPosition({left ? margin : win.width - m_w - margin, (win.height - m_h) / 2.f});

    m_bg = CCDrawNode::create();
    m_dyn = CCDrawNode::create();
    m_glow = CCDrawNode::create();
    m_glow->setBlendFunc({GL_SRC_ALPHA, GL_ONE}); // additive = neon glow
    this->addChild(m_bg, 0);
    this->addChild(m_dyn, 1);
    this->addChild(m_glow, 2);

    m_label = CCLabelBMFont::create("", "bigFont.fnt");
    m_label->setScale(0.36f);
    m_label->setOpacity((GLubyte)(255.f * m_alpha));
    m_label->setPosition({m_w * 0.5f, m_h - 15.f});
    this->addChild(m_label, 3);

    this->drawStatic();
    this->scheduleUpdate();
    s_inst = this;
    return true;
}

void TimingBar::loadFor(std::string const& levelName) {
    m_events = jg::load(levelName);
    this->rewind(-1e9f); // reset every segment
}

void TimingBar::onButton(bool down) {
    if (down && !m_down) m_pressed = true;
    m_down = down;
}

void TimingBar::rewind(float lineX) {
    for (auto& e : m_events) {
        if (e.x1 > lineX) {
            e.hit = false;
            e.done = false;
            e.cursor = e.x0;
            e.spans.clear();
        }
    }
    m_streak = 0;
    m_first = 0;
    m_shown = -99;
}

void TimingBar::tickLogic(float lineX) {
    float pre = m_preSec * m_speed; // small early-press window
    for (size_t i = m_first; i < m_events.size(); ++i) {
        auto& e = m_events[i];
        if (e.x0 - pre > lineX) break; // sorted: everything after is further away
        if (e.done) continue;
        bool reached = lineX >= e.x0;

        if (e.hold) {
            if (reached) {
                float to = std::min(lineX, e.x1);
                if (to > e.cursor) {
                    if (!e.spans.empty() && e.spans.back().ok == m_down) e.spans.back().x1 = to;
                    else e.spans.push_back({e.cursor, to, m_down});
                    e.cursor = to;
                }
                if (m_down && !e.hit) { e.hit = true; m_flash = 1.f; }
            }
        } else if (!e.hit && (m_pressed || (m_down && reached))) {
            e.hit = true;
            m_pressed = false;
            m_flash = 1.f;
        }

        if (lineX > e.x1) {
            e.done = true;
            bool ok = e.hit;
            if (e.hold) {
                float good = 0.f;
                for (auto& s : e.spans) if (s.ok) good += s.x1 - s.x0;
                ok = good >= (e.x1 - e.x0) * 0.85f;
            }
            m_streak = ok ? m_streak + 1 : 0;
        }
    }
}

void TimingBar::refreshLabel() {
    int state = m_events.empty() ? -2 : m_streak;
    if (state == m_shown) return;
    m_shown = state;
    if (state == -2) {
        m_label->setString("NO DATA");
        m_label->setColor({255, 90, 100});
    } else {
        m_label->setString(state > 0 ? fmt::format("x{}", state).c_str() : "READY");
        m_label->setColor({(GLubyte)(m_accent.r * 255.f), (GLubyte)(m_accent.g * 255.f),
                           (GLubyte)(m_accent.b * 255.f)});
    }
}

void TimingBar::update(float dt) {
    auto pl = PlayLayer::get();
    if (!pl || !pl->m_player1) return;

    float px = pl->m_player1->getPositionX();
    m_speed = jg::unitsPerSec(pl->m_player1->m_playerSpeed);
    float lineX = px + m_offsetSec * m_speed;

    if (px + 8.f < m_lastPx) this->rewind(lineX); // respawn / restart
    m_lastPx = px;

    this->tickLogic(lineX);
    m_pressed = false;
    this->drawFrame(lineX, dt);
    this->refreshLabel();
}

// ===== TimingDraw.cpp =====
using namespace jg;

static ccColor4F const GREEN = rgba(0.20f, 1.00f, 0.45f);
static ccColor4F const RED   = rgba(1.00f, 0.25f, 0.32f);
static ccColor4F const AMBER = rgba(1.00f, 0.78f, 0.20f);
static ccColor4F const WHITE = rgba(1.00f, 1.00f, 1.00f);

void TimingBar::drawStatic() {
    m_bg->clear();
    // soft outer neon halo
    for (int i = 3; i >= 1; --i) {
        roundRect(m_bg, -i * 2.f, -i * 2.f, m_w + i * 2.f, m_h + i * 2.f, 12.f + i * 2.f,
                  rgba(0, 0, 0, 0), 2.f, col(m_accent, 0.06f * (4 - i)));
    }
    // panel
    roundRect(m_bg, 0, 0, m_w, m_h, 12.f, col(rgba(0.03f, 0.04f, 0.07f), 0.82f),
              1.6f, col(m_accent, 0.75f));
    // lane
    roundRect(m_bg, m_pad, m_yBot, m_w - m_pad, m_yTop, 8.f,
              col(rgba(0.01f, 0.02f, 0.04f), 0.70f), 1.f, col(WHITE, 0.07f));
    // exit zone under the line (where the green/red trail shows)
    roundRect(m_bg, m_pad + 1, m_yBot + 1, m_w - m_pad - 1, m_yLine - 2, 6.f, col(m_accent, 0.06f));
    // header divider
    m_bg->drawSegment({m_pad + 6, m_yTop + 6}, {m_w - m_pad - 6, m_yTop + 6}, 0.6f,
                      col(m_accent, 0.4f));
}

void TimingBar::glowRect(float x0, float y0, float x1, float y1, ccColor4F c) {
    for (int i = 0; i < 3; ++i) {
        float g = 2.f + i * 3.f;
        roundRect(m_glow, x0 - g, y0 - g, x1 + g, y1 + g, 6.f + g, col(c, 0.10f - i * 0.03f));
    }
}

void TimingBar::drawEvent(Evt const& e, float ya, float yb) {
    float x0 = m_pad + 5.f, x1 = m_w - m_pad - 5.f;
    float lo = std::max(ya, m_yBot + 1.f), hi = std::min(yb, m_yTop - 1.f);
    if (hi - lo < 5.f) hi = lo + 5.f;
    bool atLine = ya <= m_yLine + 4.f && yb >= m_yLine - 4.f;
    float pulse = 0.65f + 0.35f * std::sin(m_time * 14.f);

    ccColor4F fill = rgba(0.17f, 0.18f, 0.22f); // dark grey = waiting
    ccColor4F rim = rgba(0.42f, 0.44f, 0.54f);
    ccColor4F glow = rgba(0, 0, 0, 0);

    if (!e.hold) {
        if (e.hit)       { fill = rgba(0.06f, 0.50f, 0.22f); rim = GREEN; glow = GREEN; }
        else if (e.done) { fill = rgba(0.42f, 0.09f, 0.12f); rim = RED; }
        else if (atLine) { fill = rgba(0.50f * pulse, 0.36f * pulse, 0.06f); rim = AMBER; glow = AMBER; }
    } else if (atLine) {
        rim = m_accent;
    }

    roundRect(m_dyn, x0, lo, x1, hi, 5.f, col(fill, 0.96f), 1.4f, col(rim));
    if (m_glowOn && glow.a > 0.f) glowRect(x0, lo, x1, hi, glow);
    if (!e.hold) return;

    // hold segments: down chevrons = "keep holding"
    float cx = m_w * 0.5f;
    for (float y = lo + 9.f; y < hi - 6.f; y += 15.f) {
        tri(m_dyn, {cx - 6, y + 3}, {cx + 6, y + 3}, {cx, y - 4}, col(WHITE, 0.16f));
    }
    // already-passed part: green while held, red when released
    for (auto& s : e.spans) {
        float sa = std::max(yOf(s.x0), m_yBot + 1.f);
        float sb = std::min(yOf(s.x1), m_yTop - 1.f);
        if (sb - sa < 1.f) continue;
        roundRect(m_dyn, x0 + 3, sa, x1 - 3, sb, 3.f, col(s.ok ? GREEN : RED, 0.9f));
        if (m_glowOn && s.ok) glowRect(x0 + 3, sa, x1 - 3, sb, GREEN);
    }
}

void TimingBar::drawLine(float dt) {
    ccColor4F lc = m_down ? GREEN : m_accent;
    float pulse = 0.6f + 0.4f * std::sin(m_time * 6.f);
    float zone = std::min(m_preSec * m_speed * m_pxU, 46.f);

    roundRect(m_dyn, m_pad + 1, m_yLine, m_w - m_pad - 1, m_yLine + zone, 2.f, col(lc, 0.10f));
    m_dyn->drawSegment({m_pad, m_yLine}, {m_w - m_pad, m_yLine}, 1.5f, col(lc));
    if (m_glowOn) {
        m_glow->drawSegment({m_pad, m_yLine}, {m_w - m_pad, m_yLine}, 5.f, col(lc, 0.20f * pulse));
        m_glow->drawSegment({m_pad, m_yLine}, {m_w - m_pad, m_yLine}, 10.f, col(lc, 0.07f * pulse));
    }
    tri(m_dyn, {1, m_yLine + 7}, {1, m_yLine - 7}, {10, m_yLine}, col(lc));
    tri(m_dyn, {m_w - 1, m_yLine + 7}, {m_w - 1, m_yLine - 7}, {m_w - 10, m_yLine}, col(lc));

    if (m_flash > 0.f) { // ripple when a press lands
        float k = 1.f - m_flash;
        m_glow->drawSegment({m_pad, m_yLine}, {m_w - m_pad, m_yLine}, 4.f + k * 22.f,
                            col(GREEN, 0.35f * m_flash));
        m_flash = std::max(0.f, m_flash - dt * 3.5f);
    }
}

void TimingBar::drawFrame(float lineX, float dt) {
    m_lineX = lineX;
    m_time += dt;
    m_dyn->clear();
    m_glow->clear();

    float xBottom = lineX - (m_yLine - m_yBot) / m_pxU; // world x at the lane bottom
    if (m_grid) {
        float step = 120.f; // every 4 blocks, scrolls with your speed
        float x = std::floor(xBottom / step) * step;
        for (int n = 0; n < 64; ++n, x += step) {
            float y = yOf(x);
            if (y > m_yTop) break;
            if (y < m_yBot) continue;
            m_dyn->drawSegment({m_pad + 3, y}, {m_w - m_pad - 3, y}, 0.5f, col(WHITE, 0.08f));
        }
    }

    while (m_first < m_events.size() && m_events[m_first].x1 < xBottom - 10.f) ++m_first;
    for (size_t i = m_first; i < m_events.size(); ++i) {
        auto& e = m_events[i];
        float ya = yOf(e.x0), yb = yOf(e.x1);
        if (ya > m_yTop) break;
        if (yb < m_yBot) continue;
        this->drawEvent(e, ya, yb);
    }

    this->drawLine(dt);

    // fade at the top so segments emerge out of the dark
    float fh = (m_yTop - m_yLine) * 0.22f;
    for (int i = 0; i < 8; ++i) {
        float t = i / 8.f;
        roundRect(m_dyn, m_pad + 1, m_yTop - fh * (t + 0.125f), m_w - m_pad - 1, m_yTop - fh * t,
                  0.5f, col(rgba(0.02f, 0.03f, 0.05f), 0.85f * (1.f - t)));
    }
}

// ===== main.cpp =====
// Editor: every time you press Copy, the copied objects become bar segments
class $modify(JGEditorUI, EditorUI) {
    void onCopy(CCObject* sender) {
        EditorUI::onCopy(sender);
        std::string data;
        if (auto sel = this->getSelectedObjects()) {
            for (auto obj : CCArrayExt<GameObject*>(sel)) {
                data += std::string(obj->getSaveString(m_editorLayer));
                data += ';';
            }
        }
        if (data.empty()) return;
        auto evts = jg::buildEvents(data);
        auto name = std::string(m_editorLayer->m_level->m_levelName);
        jg::store(name, evts);
        Notification::create(fmt::format("Jump Guide: {} segments saved", evts.size()),
                             NotificationIcon::Success, 1.2f)->show();
    }
};

// Playing: put the bar on the UI layer
class $modify(JGPlayLayer, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
        if (!jg::flag("enabled")) return true;

        auto bar = TimingBar::create();
        if (!bar) return true;
        bar->loadFor(std::string(level->m_levelName));
        m_uiLayer->addChild(bar, 50);
        return true;
    }
};

// Jump button (button 1) feeds the bar
class $modify(JGBaseLayer, GJBaseGameLayer) {
    void handleButton(bool down, int button, bool isPlayer1) {
        GJBaseGameLayer::handleButton(down, button, isPlayer1);
        if (button == 1) {
            if (auto bar = TimingBar::get()) bar->onButton(down);
        }
    }
};

