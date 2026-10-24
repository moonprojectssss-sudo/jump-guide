#include "Menu.hpp"
using namespace jg;

static ccColor4F const DARK = rgba(0.15f, 0.15f, 0.16f);
static ccColor3B const T_INK = c3(0.12f, 0.12f, 0.13f);
static ccColor3B const T_MUTE = c3(0.46f, 0.46f, 0.49f);
static ccColor3B const T_WHITE = c3(1.f, 1.f, 1.f);

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
    m_hits.push_back(Hit{id, CCRect(x0, y0, x1 - x0, y1 - y0), std::move(fn)});
}

void JGMenu::cardAt(float y) {
    roundRect(m_gp, kX0, y, kX1, y + kRH, 7.f, A(rgba(1.f, 1.f, 1.f)), 1.f,
              A(rgba(0.86f, 0.86f, 0.88f)));
}

void JGMenu::rowText(std::string const& id, std::string const& txt, float y) {
    cardAt(y);
    lab(id, txt, false, kX0 + 10.f, y + kRH * 0.5f, 0.6f, T_INK, 0.f);
}

void JGMenu::rowInfo(std::string const& id, std::string const& label, std::string const& value,
                     float y) {
    cardAt(y);
    lab(id + "_l", label, false, kX0 + 10.f, y + kRH * 0.5f, 0.6f, T_MUTE, 0.f);
    lab(id + "_v", value, false, kX1 - 10.f, y + kRH * 0.5f, 0.6f, T_INK, 1.f);
}

// Checklist box, writes straight into the mod settings
void JGMenu::rowCheck(std::string const& id, std::string const& label, std::string const& key,
                      float y) {
    bool on = Mod::get()->getSettingValue<bool>(key);
    float t = anim(id, on ? 1.f : 0.f, 16.f);
    float p = anim(id + "_p", m_pressId == id ? 1.f : 0.f, 22.f);
    cardAt(y);
    if (p > 0.02f) {
        roundRect(m_gp, kX0, y, kX1, y + kRH, 7.f, A(rgba(0.f, 0.f, 0.f), 0.06f * p));
    }
    lab(id + "_l", label, false, kX0 + 10.f, y + kRH * 0.5f, 0.62f, T_INK, 0.f);

    float bx = kX1 - 24.f, by = y + 4.f;
    roundRect(m_gp, bx, by, bx + 14.f, by + 14.f, 4.f, A(mixc(rgba(1.f, 1.f, 1.f), DARK, t)),
              1.4f, A(DARK));
    if (t > 0.05f) {
        m_gp->drawSegment(CCPoint(bx + 3.2f, by + 7.2f), CCPoint(bx + 6.0f, by + 4.2f), 1.5f,
                          A(rgba(1.f, 1.f, 1.f), t));
        m_gp->drawSegment(CCPoint(bx + 6.0f, by + 4.2f), CCPoint(bx + 11.0f, by + 10.2f), 1.5f,
                          A(rgba(1.f, 1.f, 1.f), t));
    }
    std::string k = key;
    this->addHit(id, kX0, y, kX1, y + kRH, [k] {
        auto* m = Mod::get();
        m->setSettingValue<bool>(k, !m->getSettingValue<bool>(k));
        jg::applyLive();
    });
}

void JGMenu::rowButton(std::string const& id, std::string const& label, float y, bool primary,
                       std::function<void()> fn) {
    float p = anim(id + "_p", m_pressId == id ? 1.f : 0.f, 22.f);
    float in = p * 2.f;
    ccColor4F base = primary ? DARK : rgba(0.80f, 0.80f, 0.82f);
    ccColor4F down = primary ? rgba(0.33f, 0.33f, 0.35f) : rgba(0.68f, 0.68f, 0.70f);
    roundRect(m_gp, kX0 + in, y + in * 0.5f, kX1 - in, y + kRH - in * 0.5f, 7.f,
              A(mixc(base, down, p)));
    lab(id + "_l", label, false, (kX0 + kX1) * 0.5f, y + kRH * 0.5f, 0.64f,
        primary ? T_WHITE : T_INK, 0.5f);
    this->addHit(id, kX0, y, kX1, y + kRH, std::move(fn));
}

// [-] value [+]
void JGMenu::rowStep(std::string const& id, std::string const& label, std::string const& key,
                     bool isInt, double lo, double hi, double step, std::string const& suffix,
                     float y) {
    auto* m = Mod::get();
    double v = isInt ? (double)m->getSettingValue<int64_t>(key) : m->getSettingValue<double>(key);
    cardAt(y);
    lab(id + "_l", label, false, kX0 + 10.f, y + kRH * 0.5f, 0.62f, T_INK, 0.f);

    float my = y + kRH * 0.5f, mx = kX1 - 88.f, px = kX1 - 16.f;
    for (int d = 0; d < 2; ++d) {
        std::string bid = id + (d == 0 ? "_m" : "_p");
        float cx = d == 0 ? mx : px;
        float pr = anim(bid + "a", m_pressId == bid ? 1.f : 0.f, 22.f);
        m_gp->drawDot(CCPoint(cx, my), 8.5f - pr * 1.5f,
                      A(mixc(DARK, rgba(0.38f, 0.38f, 0.40f), pr)));
        m_gp->drawSegment(CCPoint(cx - 3.5f, my), CCPoint(cx + 3.5f, my), 1.1f,
                          A(rgba(1.f, 1.f, 1.f)));
        if (d == 1) {
            m_gp->drawSegment(CCPoint(cx, my - 3.5f), CCPoint(cx, my + 3.5f), 1.1f,
                              A(rgba(1.f, 1.f, 1.f)));
        }
        std::string k = key;
        int dir = d == 0 ? -1 : 1;
        this->addHit(bid, cx - 11.f, y, cx + 11.f, y + kRH, [k, isInt, lo, hi, step, dir] {
            auto* mm = Mod::get();
            double cur = isInt ? (double)mm->getSettingValue<int64_t>(k)
                               : mm->getSettingValue<double>(k);
            double nv = std::clamp(cur + dir * step, lo, hi);
            if (isInt) mm->setSettingValue<int64_t>(k, (int64_t)std::llround(nv));
            else mm->setSettingValue<double>(k, nv);
            jg::applyLive();
        });
    }
    std::string txt = isInt ? fmt::format("{}{}", (long long)v, suffix)
                            : fmt::format("{:.0f}{}", v, suffix);
    lab(id + "_v", txt, false, (mx + px) * 0.5f, my, 0.58f, T_INK, 0.5f);
}

// Tap to cycle through options
void JGMenu::rowCycle(std::string const& id, std::string const& label, std::string const& key,
                      std::vector<std::string> const& opts, float y) {
    std::string cur = Mod::get()->getSettingValue<std::string>(key);
    float p = anim(id + "_p", m_pressId == id ? 1.f : 0.f, 22.f);
    cardAt(y);
    if (p > 0.02f) {
        roundRect(m_gp, kX0, y, kX1, y + kRH, 7.f, A(rgba(0.f, 0.f, 0.f), 0.06f * p));
    }
    lab(id + "_l", label, false, kX0 + 10.f, y + kRH * 0.5f, 0.62f, T_INK, 0.f);
    lab(id + "_v", cur, false, kX1 - 26.f, y + kRH * 0.5f, 0.62f, T_MUTE, 1.f);
    tri(m_gp, CCPoint(kX1 - 17.f, y + 6.f), CCPoint(kX1 - 17.f, y + 16.f),
        CCPoint(kX1 - 11.f, y + 11.f), A(DARK));

    std::string k = key;
    auto o = opts;
    this->addHit(id, kX0, y, kX1, y + kRH, [k, o] {
        auto* m = Mod::get();
        std::string c = m->getSettingValue<std::string>(k);
        size_t i = 0;
        for (; i < o.size(); ++i) if (o[i] == c) break;
        m->setSettingValue<std::string>(k, o[(i + 1) % o.size()]);
        jg::applyLive();
    });
}

// ---------- the panel ----------

void JGMenu::drawPanel() {
    m_gp->clear();
    m_hits.clear();
    m_used.clear();

    // soft shadow + body
    for (int i = 4; i >= 1; --i) {
        roundRect(m_gp, -i * 3.f, -i * 3.f - 3.f, PW + i * 3.f, PH + i * 3.f - 3.f,
                  18.f + i * 3.f, A(rgba(0.f, 0.f, 0.f), 0.045f));
    }
    roundRect(m_gp, 0.f, 0.f, PW, PH, 18.f, A(rgba(0.925f, 0.925f, 0.935f)), 1.4f,
              A(rgba(1.f, 1.f, 1.f)));

    // narrow rail with the menu choices
    roundRect(m_gp, 8.f, 8.f, 72.f, PH - 8.f, 13.f, A(DARK));
    lab("logo", "JG", true, 40.f, PH - 27.f, 0.5f, T_WHITE, 0.5f);

    static char const* names[5] = {"Guide", "Style", "Info", "Level", "More"};
    m_tabY += (tabY(m_tab) - m_tabY) * std::min(1.f, m_dt * 16.f);
    roundRect(m_gp, 14.f, m_tabY - 17.f, 66.f, m_tabY + 17.f, 10.f, A(rgba(0.97f, 0.97f, 0.98f)));
    for (int i = 0; i < 5; ++i) {
        float cy = tabY(i);
        std::string tid = "tab" + std::to_string(i);
        float on = anim(tid, m_tab == i ? 1.f : 0.f, 14.f);
        float v = lerpf(0.74f, 0.12f, on);
        lab(tid + "_l", names[i], false, 40.f, cy, 0.62f, c3(v, v, v), 0.5f);
        this->addHit(tid, 14.f, cy - 17.f, 66.f, cy + 17.f, [this, i] { m_tab = i; });
    }

    // title + close button
    static char const* titles[5] = {"Jump Guide", "Bar style", "Info overlay", "Level info",
                                    "More tools"};
    lab("title", titles[m_tab], false, kX0 + 2.f, PH - 26.f, 0.95f, T_INK, 0.f);

    float ccx = PW - 26.f, ccy = PH - 25.f;
    float pr = anim("close_a", m_pressId == "close" ? 1.f : 0.f, 20.f);
    m_gp->drawDot(CCPoint(ccx, ccy), 12.f - pr * 1.5f, A(mixc(DARK, rgba(0.38f, 0.38f, 0.40f), pr)));
    m_gp->drawSegment(CCPoint(ccx - 4.f, ccy - 4.f), CCPoint(ccx + 4.f, ccy + 4.f), 1.3f,
                      A(rgba(1.f, 1.f, 1.f)));
    m_gp->drawSegment(CCPoint(ccx - 4.f, ccy + 4.f), CCPoint(ccx + 4.f, ccy - 4.f), 1.3f,
                      A(rgba(1.f, 1.f, 1.f)));
    this->addHit("close", ccx - 16.f, ccy - 16.f, ccx + 16.f, ccy + 16.f, [this] { closeMenu(); });

    this->tabContent();

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

    switch (m_tab) {
    case 0: { // Guide
        std::string msg = !m_status.empty() ? m_status
            : (have ? fmt::format("{} segments ready", jg::state.segs)
                    : std::string("Level not read yet"));
        this->rowText("g_st", msg, rowY(0));
        this->rowButton("g_read", "Read this level now", rowY(1), true, [this] { readLevel(); });
        this->rowCheck("g_en", "Timing bar", "enabled", rowY(2));
        this->rowCheck("g_up", "Upright spikes only", "upright-only", rowY(3));
        lab("g_tip", "Tip: mark hold parts with color channel 1000", false, kX0 + 4.f,
            rowY(4) + 11.f, 0.5f, T_MUTE, 0.f);
        break;
    }
    case 1: { // Style
        this->rowCycle("s_side", "Bar side", "side", {"Right", "Left"}, rowY(0));
        this->rowCycle("s_acc", "Accent color", "accent",
                       {"Cyan", "Magenta", "Lime", "Amber", "Violet", "Silver"}, rowY(1));
        this->rowStep("s_op", "Opacity", "opacity", false, 20, 100, 10, "%", rowY(2));
        this->rowStep("s_w", "Bar width", "bar-width", true, 32, 120, 8, " px", rowY(3));
        this->rowStep("s_v", "View distance", "view-distance", false, 6, 40, 2, " bl", rowY(4));
        this->rowStep("s_o", "Press offset", "press-offset", true, 0, 300, 10, " ms", rowY(5));
        this->rowCheck("s_gl", "Glow effects", "glow", rowY(6));
        this->rowCheck("s_gr", "Scrolling grid", "scroll-grid", rowY(7));
        break;
    }
    case 2: { // Info overlay (read-only helpers shown while playing)
        this->rowCheck("h_p", "Percent", "hud-percent", rowY(0));
        this->rowCheck("h_f", "FPS", "hud-fps", rowY(1));
        this->rowCheck("h_c", "Clock", "hud-clock", rowY(2));
        this->rowCheck("h_s", "Attempts (session)", "hud-session", rowY(3));
        this->rowCheck("h_b", "Best % (session)", "hud-best", rowY(4));
        this->rowButton("h_r", "Reset session stats", rowY(5), false, [] {
            jg::state.attempts = 0;
            jg::state.best = 0;
        });
        break;
    }
    case 3: { // Level info
        this->rowInfo("i_n", "Level", name.empty() ? std::string("-") : shorten(name, 20), rowY(0));
        this->rowInfo("i_o", "Objects", have ? fmt::format("{}", st.objects) : std::string("-"), rowY(1));
        this->rowInfo("i_s", "Spikes", have ? fmt::format("{}", st.spikes) : std::string("-"), rowY(2));
        this->rowInfo("i_h", "Hold blocks", have ? fmt::format("{}", st.holds) : std::string("-"), rowY(3));
        this->rowInfo("i_l", "Length", have ? fmt::format("{:.0f} blocks", st.maxX / 30.f) : std::string("-"), rowY(4));
        this->rowInfo("i_g", "Segments", have ? fmt::format("{}", jg::state.segs) : std::string("-"), rowY(5));
        this->rowButton("i_read", "Read this level now", rowY(6), true, [this] { readLevel(); });
        break;
    }
    default: { // More
        this->rowButton("m_str", "Reset streak counter", rowY(0), false, [] {
            if (auto b = TimingBar::get()) b->resetStreak();
        });
        this->rowButton("m_ses", "Reset session stats", rowY(1), false, [] {
            jg::state.attempts = 0;
            jg::state.best = 0;
        });
        this->rowButton("m_pos", "Reset menu button position", rowY(2), false, [this] {
            m_bub = CCPoint(m_win.width - 34.f, m_win.height * 0.5f);
            Mod::get()->setSavedValue<double>("bub-x", m_bub.x);
            Mod::get()->setSavedValue<double>("bub-y", m_bub.y);
        });
        this->rowButton("m_all", "Reset all settings", rowY(3), false, [this] { resetAll(); });
        this->rowText("m_a1", "Jump Guide v1.0.0 by Arctium", rowY(5));
        this->rowText("m_a2", "Practice aid only. No cheats.", rowY(6));
        break;
    }
    }
}
