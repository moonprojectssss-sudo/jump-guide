#include "TimingBar.hpp"

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

    m_bg = CCDrawNode::create();
    m_dyn = CCDrawNode::create();
    m_glow = CCDrawNode::create();
    m_glow->setBlendFunc({GL_SRC_ALPHA, GL_ONE}); // additive = neon glow
    this->addChild(m_bg, 0);
    this->addChild(m_dyn, 1);
    this->addChild(m_glow, 2);

    m_label = CCLabelBMFont::create("", "bigFont.fnt");
    m_label->setScale(0.36f);
    this->addChild(m_label, 3);

    this->applySettings();
    this->scheduleUpdate();
    s_inst = this;
    return true;
}

// Reads every setting again, so the menu can change the bar live
void TimingBar::applySettings() {
    auto win = CCDirector::get()->getWinSize();

    m_w = (float)jg::inum("bar-width");
    m_h = win.height * (float)jg::fnum("bar-height") / 100.f;
    m_alpha = (float)jg::fnum("opacity") / 100.f;
    m_viewUnits = (float)jg::fnum("view-distance") * 30.f;
    m_offsetSec = jg::inum("press-offset") / 1000.f;
    m_grid = jg::flag("scroll-grid");
    m_hc = jg::flag("high-contrast");
    m_ladder = jg::text("bar-style") == "Ladder";
    m_reduce = jg::flag("reduce-effects");
    m_glowOn = jg::flag("glow") && !m_reduce;
    m_accent = jg::accentFor(jg::text("accent"));
    m_good = jg::goodColor();
    m_bad = jg::badColor();

    m_yLine = m_h * 0.22f;
    m_yTop = m_h - 30.f;
    m_pxU = (m_yTop - m_yLine) / m_viewUnits;

    float margin = (float)jg::inum("margin");
    bool left = jg::text("side") == "Left";
    this->setContentSize({m_w, m_h});
    this->setPosition({left ? margin : win.width - m_w - margin, (win.height - m_h) / 2.f});

    m_label->setOpacity((GLubyte)(255.f * m_alpha));
    m_label->setPosition({m_w * 0.5f, m_h - 15.f});
    m_shown = -99;

    this->setVisible(jg::flag("enabled"));
    this->drawStatic();
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
    if (!this->isVisible()) return;
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
