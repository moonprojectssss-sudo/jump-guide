#include "Menu.hpp"
using namespace jg;

static ccColor4F const GREEN = rgba(0.20f, 0.80f, 0.43f);
static ccColor4F const TRACK = rgba(0.23f, 0.24f, 0.23f);
static ccColor4F const WHITE = rgba(1.f, 1.f, 1.f);
static ccColor3B const T_WHITE = c3(1.f, 1.f, 1.f);
static ccColor3B const T_MUTE = c3(0.62f, 0.64f, 0.62f);
static ccColor3B const T_INK = c3(0.10f, 0.10f, 0.10f);

// ---------- immediate-mode helpers ----------

float JGMenu::anim(std::string const& id, float target, float speed) {
    float& v = m_anim[id];
    v += (target - v) * std::min(1.f, m_dt * speed);
    return v;
}

void JGMenu::lab(std::string const& id, std::string const& txt, bool big, float x, float y,
                 float sc, ccColor3B col, float ax) {
    auto& L = m_labs[id];
    if (!L.n) {
        L.n = CCLabelBMFont::create(txt.c_str(), big ? "bigFont.fnt" : "chatFont.fnt");
        L.s = txt;
        m_root->addChild(L.n, 1);
    } else if (L.s != txt) {
        L.n->setString(txt.c_str());
        L.s = txt;
    }
    L.n->setAnchorPoint(CCPoint(ax, 0.5f));
    L.n->setPosition(CCPoint(x, y));
    L.n->setScale(sc);
    L.n->setColor(col);
    L.n->setOpacity((GLubyte)(255.f * std::min(1.f, m_openT)));
    L.n->setVisible(true);
    m_used.insert(id);
}

void JGMenu::addHit(std::string const& id, float x0, float y0, float x1, float y1,
                    std::function<void()> fn) {
    Hit h;
    h.id = id;
    h.r = CCRect(x0, y0, x1 - x0, y1 - y0);
    h.fn = std::move(fn);
    m_hits.push_back(std::move(h));
}

// Separator line, press highlight and the row label
void JGMenu::rowBase(std::string const& id, std::string const& label, float y) {
    float p = anim(id + "_p", (m_pressId == id && !m_scrolling) ? 1.f : 0.f, 22.f);
    if (p > 0.02f) {
        roundRect(m_gp, kX0 - 4.f, y + 1.f, kX1 + 4.f, y + kRH - 1.f, 6.f, A(WHITE, 0.07f * p));
    }
    m_gp->drawSegment(CCPoint(kX0, y), CCPoint(kX1, y), 0.5f, A(WHITE, 0.07f));
    lab(id + "_l", label, true, kX0 + 4.f, y + kRH * 0.5f, 0.29f, T_WHITE, 0.f);
}

void JGMenu::rowHead(std::string const& id, std::string const& txt, float y) {
    if (!vis(y)) return;
    lab(id, txt, true, kX0 + 4.f, y + kRH * 0.5f - 3.f, 0.21f, T_MUTE, 0.f);
}

void JGMenu::rowText(std::string const& id, std::string const& txt, float y) {
    if (!vis(y)) return;
    lab(id, txt, true, kX0 + 4.f, y + kRH * 0.5f, 0.24f, T_MUTE, 0.f);
}

void JGMenu::rowInfo(std::string const& id, std::string const& label, std::string const& value,
                     float y) {
    if (!vis(y)) return;
    m_gp->drawSegment(CCPoint(kX0, y), CCPoint(kX1, y), 0.5f, A(WHITE, 0.07f));
    lab(id + "_l", label, true, kX0 + 4.f, y + kRH * 0.5f, 0.29f, T_WHITE, 0.f);
    lab(id + "_v", value, true, kX1 - 6.f, y + kRH * 0.5f, 0.26f, T_MUTE, 1.f);
}

// On/off switch, writes straight into the mod settings
void JGMenu::rowSwitch(std::string const& id, std::string const& label, std::string const& key,
                       float y) {
    if (!vis(y)) return;
    bool on = Mod::get()->getSettingValue<bool>(key);
    float t = anim(id, on ? 1.f : 0.f, 14.f);
    rowBase(id, label, y);
    float cx = kX1 - 26.f, cy = y + kRH * 0.5f;
    roundRect(m_gp, cx - 16.f, cy - 8.f, cx + 16.f, cy + 8.f, 8.f, A(mixc(TRACK, GREEN, t)));
    m_gp->drawDot(CCPoint(lerpf(cx - 8.f, cx + 8.f, t), cy), 6.3f, A(WHITE));
    std::string k = key;
    addHit(id, kX0, y, kX1, y + kRH, [k] {
        auto* m = Mod::get();
        m->setSettingValue<bool>(k, !m->getSettingValue<bool>(k));
        jg::onChanged(k);
    });
}

// Slider: tap or drag on the track
void JGMenu::rowSlider(std::string const& id, std::string const& label, std::string const& key,
                       bool isInt, double lo, double hi, double step, int dec,
                       std::string const& suffix, float y) {
    if (!vis(y)) return;
    auto* m = Mod::get();
    double v = isInt ? (double)m->getSettingValue<int64_t>(key) : m->getSettingValue<double>(key);
    float t = anim(id, (float)((v - lo) / (hi - lo)), 18.f);
    rowBase(id, label, y);

    float tx0 = kX1 - 108.f, tx1 = kX1 - 12.f, cy = y + kRH * 0.5f;
    float fx = lerpf(tx0, tx1, std::clamp(t, 0.f, 1.f));
    m_gp->drawSegment(CCPoint(tx0, cy), CCPoint(tx1, cy), 2.f, A(TRACK));
    m_gp->drawSegment(CCPoint(tx0, cy), CCPoint(fx, cy), 2.f, A(WHITE));
    m_gp->drawDot(CCPoint(fx, cy), 6.f, A(WHITE));
    lab(id + "_v", fmt::format("{:.{}f}{}", v, dec, suffix), true, tx0 - 10.f, cy, 0.24f, T_WHITE, 1.f);

    std::string k = key;
    Hit h;
    h.id = id;
    h.r = CCRect(tx0 - 10.f, y, (tx1 - tx0) + 20.f, kRH);
    h.slider = true;
    h.sx0 = tx0;
    h.sx1 = tx1;
    h.sfn = [k, isInt, lo, hi, step](float u) {
        auto* mm = Mod::get();
        double raw = lo + std::clamp(u, 0.f, 1.f) * (hi - lo);
        double nv = std::clamp(lo + std::round((raw - lo) / step) * step, lo, hi);
        if (isInt) {
            int64_t iv = (int64_t)std::llround(nv);
            if (mm->getSettingValue<int64_t>(k) != iv) {
                mm->setSettingValue<int64_t>(k, iv);
                jg::onChanged(k);
            }
        } else if (std::fabs(mm->getSettingValue<double>(k) - nv) > 1e-6) {
            mm->setSettingValue<double>(k, nv);
            jg::onChanged(k);
        }
    };
    m_hits.push_back(std::move(h));
}

// Tap to cycle through options
void JGMenu::rowCycle(std::string const& id, std::string const& label, std::string const& key,
                      std::vector<std::string> const& opts, float y) {
    if (!vis(y)) return;
    std::string cur = Mod::get()->getSettingValue<std::string>(key);
    rowBase(id, label, y);
    float w = 100.f, cx = kX1 - w * 0.5f - 4.f, cy = y + kRH * 0.5f;
    roundRect(m_gp, cx - w * 0.5f, cy - 9.f, cx + w * 0.5f, cy + 9.f, 9.f, A(rgba(0.18f, 0.19f, 0.18f)),
              0.8f, A(WHITE, 0.12f));
    lab(id + "_v", cur, true, cx - 6.f, cy, 0.22f, T_WHITE, 0.5f);
    float ax = cx + w * 0.5f - 11.f;
    tri(m_gp, CCPoint(ax - 4.f, cy + 2.5f), CCPoint(ax + 4.f, cy + 2.5f), CCPoint(ax, cy - 2.5f), A(WHITE, 0.8f));

    std::string k = key;
    auto o = opts;
    addHit(id, kX0, y, kX1, y + kRH, [k, o] {
        auto* m = Mod::get();
        std::string c = m->getSettingValue<std::string>(k);
        size_t i = 0;
        for (; i < o.size(); ++i) if (o[i] == c) break;
        m->setSettingValue<std::string>(k, o[(i + 1) % o.size()]);
        jg::onChanged(k);
    });
}

// Label on the left, small white button on the right
void JGMenu::rowAction(std::string const& id, std::string const& label, std::string const& btn,
                       float y, std::function<void()> fn) {
    if (!vis(y)) return;
    rowBase(id, label, y);
    float w = 64.f, cx = kX1 - w * 0.5f - 4.f, cy = y + kRH * 0.5f;
    float p = anim(id + "_a", (m_pressId == id && !m_scrolling) ? 1.f : 0.f, 22.f);
    roundRect(m_gp, cx - w * 0.5f + p, cy - 9.f + p * 0.5f, cx + w * 0.5f - p, cy + 9.f - p * 0.5f, 9.f,
              A(mixc(WHITE, rgba(0.75f, 0.75f, 0.75f), p)));
    lab(id + "_b", btn, true, cx, cy, 0.23f, T_INK, 0.5f);
    addHit(id, kX0, y, kX1, y + kRH, std::move(fn));
}

// Where runs ended, per 10% (3 rows tall)
void JGMenu::chartHeat(float y) {
    float h = kRH * 3.f;
    if (!vis(y, h)) return;
    float x0 = kX0 + 6.f, x1 = kX1 - 6.f, base = y + 16.f, hh = h - 28.f;
    int mx = 0, top = 0;
    for (int b = 0; b < 10; ++b) {
        if (jg::state.bins[b] > mx) { mx = jg::state.bins[b]; top = b; }
    }
    m_gp->drawSegment(CCPoint(x0, base), CCPoint(x1, base), 0.6f, A(WHITE, 0.15f));
    if (mx == 0) {
        lab("ch_hn", "No attempts yet", true, (x0 + x1) * 0.5f, y + h * 0.5f, 0.24f, T_MUTE, 0.5f);
    } else {
        float w = (x1 - x0) / 10.f;
        for (int b = 0; b < 10; ++b) {
            float bh = std::max(2.f, hh * jg::state.bins[b] / (float)mx);
            ccColor4F c = b == top ? badColor() : A(WHITE, 0.75f);
            if (b == top) c.a *= m_openT;
            roundRect(m_gp, x0 + b * w + 3.f, base, x0 + (b + 1) * w - 3.f, base + bh, 3.f, c);
        }
    }
    lab("ch_h0", "0%", true, x0, y + 7.f, 0.18f, T_MUTE, 0.f);
    lab("ch_h1", "100%", true, x1, y + 7.f, 0.18f, T_MUTE, 1.f);
}

// Last 20 runs as a line graph (3 rows tall)
void JGMenu::chartGraph(float y) {
    float h = kRH * 3.f;
    if (!vis(y, h)) return;
    float x0 = kX0 + 6.f, x1 = kX1 - 6.f, base = y + 8.f, hh = h - 16.f;
    for (int g = 0; g <= 2; ++g) {
        float gy = base + hh * g * 0.5f;
        m_gp->drawSegment(CCPoint(x0, gy), CCPoint(x1, gy), 0.5f, A(WHITE, 0.10f));
    }
    auto& hist = jg::state.history;
    if (hist.size() < 2) {
        lab("ch_gn", "Play a few attempts first", true, (x0 + x1) * 0.5f, y + h * 0.5f, 0.24f, T_MUTE, 0.5f);
        return;
    }
    float dx = (x1 - x0) / (float)(hist.size() - 1);
    CCPoint prev;
    for (size_t i = 0; i < hist.size(); ++i) {
        CCPoint p(x0 + dx * i, base + hh * hist[i] / 100.f);
        if (i > 0) m_gp->drawSegment(prev, p, 1.2f, A(WHITE, 0.85f));
        prev = p;
    }
    for (size_t i = 0; i < hist.size(); ++i) {
        m_gp->drawDot(CCPoint(x0 + dx * i, base + hh * hist[i] / 100.f), 2.4f, A(GREEN));
    }
}

// ---------- the panel ----------

void JGMenu::drawPanel() {
    m_gp->clear();
    m_hits.clear();
    m_used.clear();
    m_scroll = std::clamp(m_scroll, 0.f, m_maxScroll);

    // soft shadow + body
    for (int i = 4; i >= 1; --i) {
        roundRect(m_gp, -i * 3.f, -i * 3.f - 3.f, PW + i * 3.f, PH + i * 3.f - 3.f,
                  20.f + i * 3.f, A(rgba(0.f, 0.f, 0.f), 0.05f));
    }
    roundRect(m_gp, 0.f, 0.f, PW, PH, 20.f, A(rgba(0.055f, 0.062f, 0.058f), 0.99f), 1.2f, A(WHITE, 0.10f));

    // header: AM tile, title, version, close
    roundRect(m_gp, 14.f, PH - 37.f, 40.f, PH - 11.f, 8.f, A(WHITE));
    lab("hd_am", "AM", true, 27.f, PH - 24.f, 0.25f, T_INK, 0.5f);
    lab("hd_title", "Assist Menu", true, 48.f, PH - 24.f, 0.36f, T_WHITE, 0.f);
    lab("hd_ver", "v1.0.0", true, PW - 52.f, PH - 24.f, 0.2f, T_MUTE, 1.f);

    float ccx = PW - 26.f, ccy = PH - 24.f;
    float pr = anim("close_a", m_pressId == "close" ? 1.f : 0.f, 20.f);
    m_gp->drawDot(CCPoint(ccx, ccy), 11.f - pr * 1.5f, A(mixc(WHITE, rgba(0.7f, 0.7f, 0.7f), pr)));
    m_gp->drawSegment(CCPoint(ccx - 3.8f, ccy - 3.8f), CCPoint(ccx + 3.8f, ccy + 3.8f), 1.3f, A(rgba(0.1f, 0.1f, 0.1f)));
    m_gp->drawSegment(CCPoint(ccx - 3.8f, ccy + 3.8f), CCPoint(ccx + 3.8f, ccy - 3.8f), 1.3f, A(rgba(0.1f, 0.1f, 0.1f)));
    addHit("close", ccx - 16.f, ccy - 16.f, ccx + 16.f, ccy + 16.f, [this] { closeMenu(); });

    m_gp->drawSegment(CCPoint(10.f, PH - 44.f), CCPoint(PW - 10.f, PH - 44.f), 0.6f, A(WHITE, 0.08f));
    m_gp->drawSegment(CCPoint(84.f, 12.f), CCPoint(84.f, PH - 50.f), 0.6f, A(WHITE, 0.08f));

    // rail with the menu choices
    static char const* names[7] = {"Guide", "Practice", "Display", "Info", "Audio", "Access", "Controls"};
    m_tabY += (tabY(m_tab) - m_tabY) * std::min(1.f, m_dt * 16.f);
    roundRect(m_gp, 10.f, m_tabY - 15.f, 78.f, m_tabY + 15.f, 10.f, A(WHITE));
    for (int i = 0; i < 7; ++i) {
        float cy = tabY(i);
        std::string tid = "tab" + std::to_string(i);
        float on = anim(tid, m_tab == i ? 1.f : 0.f, 14.f);
        float v = lerpf(0.86f, 0.10f, on);
        lab(tid + "_l", names[i], false, 44.f, cy, 0.58f, c3(v, v, v), 0.5f);
        addHit(tid, 10.f, cy - 15.f, 78.f, cy + 15.f, [this, i] {
            m_tab = i;
            m_scroll = 0.f;
        });
    }

    this->tabContent();

    // scroll bar
    float viewH = kTop - kBot;
    m_maxScroll = std::max(0.f, m_contentH - viewH);
    if (m_maxScroll > 1.f) {
        float th = std::max(24.f, viewH * viewH / m_contentH);
        float ty1 = kTop - (m_scroll / m_maxScroll) * (viewH - th);
        m_gp->drawSegment(CCPoint(PW - 8.f, kBot), CCPoint(PW - 8.f, kTop), 1.2f, A(WHITE, 0.07f));
        m_gp->drawSegment(CCPoint(PW - 8.f, ty1 - th), CCPoint(PW - 8.f, ty1), 1.6f, A(WHITE, 0.45f));
    }

    for (auto& [id, L] : m_labs) {
        if (L.n && !m_used.count(id)) L.n->setVisible(false);
    }
}

static std::string shorten(std::string const& s, size_t n) {
    return s.size() <= n ? s : s.substr(0, n - 1) + "~";
}

void JGMenu::tabContent() {
    std::string name = this->levelName();
    bool have = jg::state.has && jg::state.name == name;
    auto& st = jg::state.stats;
    int i = 0;

    switch (m_tab) {
    case 0: { // Guide: the timing bar
        std::string msg = !m_status.empty() ? m_status
            : (have ? fmt::format("{} segments ready", jg::state.segs) : std::string("Level not read yet"));
        rowHead("g_h1", "TIMING GUIDE", rowY(i++));
        rowInfo("g_st", "Level data", msg, rowY(i++));
        rowAction("g_read", "Read this level", "Read", rowY(i++), [this] { readLevel(); });
        rowSwitch("g_en", "Timing bar", "enabled", rowY(i++));
        rowSwitch("g_up", "Upright spikes only", "upright-only", rowY(i++));
        rowHead("g_h2", "STYLE", rowY(i++));
        rowCycle("g_side", "Bar side", "side", {"Right", "Left"}, rowY(i++));
        rowCycle("g_acc", "Accent color", "accent", {"Cyan", "Magenta", "Lime", "Amber", "Violet", "Silver"}, rowY(i++));
        rowSlider("g_op", "Opacity", "opacity", false, 20, 100, 10, 0, "%", rowY(i++));
        rowSlider("g_w", "Bar width", "bar-width", true, 32, 120, 8, 0, " px", rowY(i++));
        rowSlider("g_v", "View distance", "view-distance", false, 6, 40, 2, 0, " bl", rowY(i++));
        rowSlider("g_o", "Press offset", "press-offset", true, 0, 300, 10, 0, " ms", rowY(i++));
        rowSwitch("g_gl", "Glow effects", "glow", rowY(i++));
        rowSwitch("g_gr", "Scrolling grid", "scroll-grid", rowY(i++));
        rowText("g_tip", "Tip: mark hold parts with color channel 1000", rowY(i++));
        break;
    }
    case 1: { // Practice
        rowHead("p_h1", "CHECKPOINTS", rowY(i++));
        rowSwitch("p_ac", "Auto checkpoints", "auto-checkpoints", rowY(i++));
        rowSlider("p_ct", "Checkpoint timer", "checkpoint-timer", false, 1, 10, 0.5, 1, "s", rowY(i++));
        rowText("p_n1", "Practice mode only. Normal runs are never touched.", rowY(i++));
        rowHead("p_h2", "MARKERS", rowY(i++));
        rowSwitch("p_dm", "Death markers", "death-markers", rowY(i++));
        rowAction("p_cl", "Clear markers", "Clear", rowY(i++), [this] {
            if (m_inPause && PlayLayer::get()) {
                jg::state.markerPts.clear();
                jg::redrawMarkers();
            }
        });
        rowText("p_n2", "A red cross shows where an attempt ended.", rowY(i++));
        break;
    }
    case 2: { // Display
        rowHead("d_h1", "HUD", rowY(i++));
        rowSwitch("d_p", "Percent", "hud-percent", rowY(i++));
        rowSwitch("d_f", "FPS counter", "hud-fps", rowY(i++));
        rowSwitch("d_a", "Attempt counter", "hud-session", rowY(i++));
        rowSwitch("d_t", "Show timer", "hud-timer", rowY(i++));
        rowSwitch("d_c", "Clock", "hud-clock", rowY(i++));
        rowSwitch("d_pb", "Progress bar", "hud-progress", rowY(i++));
        rowCycle("d_pos", "HUD position", "hud-pos", {"Top left", "Top right", "Bottom left", "Bottom right"}, rowY(i++));
        rowSlider("d_sz", "HUD size", "hud-size", false, 0.6, 1.6, 0.1, 1, "x", rowY(i++));
        break;
    }
    case 3: { // Info
        rowHead("n_h1", "THIS SESSION", rowY(i++));
        rowInfo("n_a", "Attempts", fmt::format("{}", jg::state.attempts), rowY(i++));
        rowInfo("n_b", "Best run", fmt::format("{}%", jg::state.best), rowY(i++));
        rowInfo("n_k", "Clicks", fmt::format("{}", jg::state.clicks), rowY(i++));
        rowHead("n_h2", "OVERLAY", rowY(i++));
        rowSwitch("n_bm", "Best run marker", "hud-bestmark", rowY(i++));
        rowSwitch("n_bs", "Best percent", "hud-best", rowY(i++));
        rowSwitch("n_ck", "Click counter", "hud-clicks", rowY(i++));
        rowSwitch("n_cp", "CPS meter", "hud-cps", rowY(i++));
        rowHead("n_h3", "WHERE RUNS ENDED", rowY(i++));
        chartHeat(rowY(i + 2));
        i += 3;
        rowHead("n_h4", "LAST 20 RUNS", rowY(i++));
        chartGraph(rowY(i + 2));
        i += 3;
        rowHead("n_h5", "LEVEL INFO", rowY(i++));
        rowInfo("n_ln", "Level", name.empty() ? std::string("-") : shorten(name, 22), rowY(i++));
        rowInfo("n_lo", "Objects", have ? fmt::format("{}", st.objects) : std::string("-"), rowY(i++));
        rowInfo("n_ls", "Spikes", have ? fmt::format("{}", st.spikes) : std::string("-"), rowY(i++));
        rowInfo("n_lh", "Hold blocks", have ? fmt::format("{}", st.holds) : std::string("-"), rowY(i++));
        rowInfo("n_ll", "Length", have ? fmt::format("{:.0f} blocks", st.maxX / 30.f) : std::string("-"), rowY(i++));
        rowAction("n_lr", "Read this level", "Read", rowY(i++), [this] { readLevel(); });
        break;
    }
    case 4: { // Audio
        rowHead("a_h1", "VOLUME", rowY(i++));
        rowSlider("a_mv", "Music volume", "music-volume", true, 0, 100, 5, 0, "%", rowY(i++));
        rowSlider("a_sv", "SFX volume", "sfx-volume", true, 0, 100, 5, 0, "%", rowY(i++));
        rowText("a_n1", "Changes the volume of the whole game.", rowY(i++));
        rowHead("a_h2", "FEEDBACK", rowY(i++));
        rowSwitch("a_cs", "Click sounds", "click-sound", rowY(i++));
        break;
    }
    case 5: { // Access
        rowHead("x_h1", "VISION", rowY(i++));
        rowCycle("x_cb", "Colorblind mode", "colorblind", {"Off", "Deutan", "Protan", "Tritan"}, rowY(i++));
        rowSwitch("x_hc", "High contrast", "high-contrast", rowY(i++));
        rowSwitch("x_lt", "Large text", "large-text", rowY(i++));
        rowSlider("x_ui", "UI scale", "ui-scale", true, 70, 130, 10, 0, "%", rowY(i++));
        rowHead("x_h2", "MOTION", rowY(i++));
        rowSwitch("x_re", "Reduce effects", "reduce-effects", rowY(i++));
        rowText("x_n1", "Applies to the Assist Menu, overlay and timing bar.", rowY(i++));
        break;
    }
    default: { // Controls
        rowHead("c_h1", "MENU BUTTON", rowY(i++));
        rowSwitch("c_sm", "Show menu button", "show-menu", rowY(i++));
        rowAction("c_rp", "Reset button position", "Reset", rowY(i++), [this] {
            m_bub = CCPoint(m_win.width - 34.f, m_win.height * 0.5f);
            Mod::get()->setSavedValue<double>("bub-x", m_bub.x);
            Mod::get()->setSavedValue<double>("bub-y", m_bub.y);
        });
        rowHead("c_h2", "DATA", rowY(i++));
        rowAction("c_st", "Reset streak counter", "Reset", rowY(i++), [] {
            if (auto b = TimingBar::get()) b->resetStreak();
        });
        rowAction("c_ss", "Reset session stats", "Reset", rowY(i++), [] { jg::state.resetSession(); });
        rowAction("c_ra", "Reset all settings", "Reset", rowY(i++), [this] { resetAll(); });
        rowHead("c_h3", "ABOUT", rowY(i++));
        rowText("c_a1", "Assist Menu v1.0.0 by Arctium", rowY(i++));
        rowText("c_a2", "Help, not cheats. A practice aid only.", rowY(i++));
        break;
    }
    }
    m_contentH = i * kRH;
}
