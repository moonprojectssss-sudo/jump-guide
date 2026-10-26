#pragma once
#include "TimingBar.hpp"
#include <ctime>

// Read-only overlay while playing + optional practice-mode auto checkpoints.
// Nothing here changes how a level plays.
class InfoHud : public CCNode {
public:
    static InfoHud* create() {
        auto r = new InfoHud();
        if (r && r->init()) {
            r->autorelease();
            return r;
        }
        CC_SAFE_DELETE(r);
        return nullptr;
    }
    static InfoHud* get() { return s_inst; }

    ~InfoHud() override {
        if (s_inst == this) s_inst = nullptr;
    }

    void applySettings() {
        m_p = jg::flag("hud-percent");
        m_f = jg::flag("hud-fps");
        m_t = jg::flag("hud-timer");
        m_c = jg::flag("hud-clock");
        m_s = jg::flag("hud-session");
        m_b = jg::flag("hud-best");
        m_k = jg::flag("hud-clicks");
        m_cp = jg::flag("hud-cps");
        m_prog = jg::flag("hud-progress");
        m_mark = jg::flag("hud-bestmark");
        m_xy = jg::flag("hud-coords");
        m_mode = jg::flag("hud-mode");
        m_input = jg::flag("input-display");

        auto win = CCDirector::get()->getWinSize();
        std::string pos = jg::text("hud-pos");
        bool right = pos.find("right") != std::string::npos;
        bool top = pos.rfind("Top", 0) == 0;
        float sc = 0.32f * (float)jg::fnum("hud-size") * (jg::flag("large-text") ? 1.3f : 1.f) *
                   (float)jg::inum("ui-scale") / 100.f;
        m_l->setAnchorPoint({right ? 1.f : 0.f, top ? 1.f : 0.f});
        m_l->setAlignment(right ? kCCTextAlignmentRight : kCCTextAlignmentLeft);
        m_l->setPosition({right ? win.width - 18.f : 18.f, top ? win.height - 14.f : 24.f});
        m_l->setScale(sc);
        m_l->setOpacity(jg::flag("high-contrast") ? 255 : 210);
        m_acc = 1.f; // refresh on the next frame
    }

protected:
    bool init() override {
        if (!CCNode::init()) return false;
        m_l = CCLabelBMFont::create("", "bigFont.fnt");
        this->addChild(m_l, 1);
        m_bar = CCDrawNode::create();
        this->addChild(m_bar, 0);
        m_il = CCLabelBMFont::create("", "bigFont.fnt");
        m_il->setScale(0.25f);
        m_il->setVisible(false);
        this->addChild(m_il, 2);
        this->applySettings();
        this->scheduleUpdate();
        s_inst = this;
        return true;
    }

    void drawProgress(PlayLayer* pl) {
        m_bar->clear();
        if (!m_prog || !pl) return;
        auto win = CCDirector::get()->getWinSize();
        float pct = std::clamp(pl->getCurrentPercent(), 0.f, 100.f);
        jg::roundRect(m_bar, 0.f, 0.f, win.width, 4.f, 0.01f, jg::rgba(0.f, 0.f, 0.f, 0.35f));
        if (pct > 0.1f) {
            jg::roundRect(m_bar, 0.f, 0.f, win.width * pct / 100.f, 4.f, 0.01f,
                          jg::rgba(1.f, 1.f, 1.f, 0.9f));
        }
        if (m_mark && jg::state.best > 0) {
            float x = win.width * jg::state.best / 100.f;
            m_bar->drawSegment(CCPoint(x, 0.f), CCPoint(x, 10.f), 1.2f,
                               jg::rgba(1.f, 0.78f, 0.20f, 0.95f));
        }
    }

    // Small pill that lights up while you press, with the hold time
    void drawInput() {
        if (!m_input) {
            m_il->setVisible(false);
            return;
        }
        auto win = CCDirector::get()->getWinSize();
        bool down = jg::state.down;
        float held = down ? (float)(jg::nowSec() - jg::state.downSince) : 0.f;
        float cx = win.width * 0.5f, cy = 28.f;
        jg::roundRect(m_bar, cx - 40.f, cy - 10.f, cx + 40.f, cy + 10.f, 10.f,
                      down ? jg::rgba(1.f, 1.f, 1.f, 0.92f) : jg::rgba(0.f, 0.f, 0.f, 0.40f), 1.f,
                      jg::rgba(1.f, 1.f, 1.f, 0.35f));
        std::string s = down ? fmt::format("HOLD {:.1f}s", held) : std::string("INPUT");
        if (s != m_ilText) {
            m_il->setString(s.c_str());
            m_ilText = s;
        }
        m_il->setPosition(CCPoint(cx, cy));
        m_il->setColor(down ? jg::c3(0.1f, 0.1f, 0.1f) : jg::c3(1.f, 1.f, 1.f));
        m_il->setVisible(true);
    }

    void update(float dt) override {
        auto pl = PlayLayer::get();
        bool has = pl && pl->m_player1;
        float px = has ? pl->m_player1->getPositionX() : 0.f;
        float py = has ? pl->m_player1->getPositionY() : 0.f;
        float dpx = px - m_lastPx, dpy = py - m_lastPy;
        bool moving = std::fabs(dpx) > 0.01f;
        if (px + 8.f < m_lastPx) { m_timer = 0.f; m_cpAcc = 0.f; } // new attempt
        if (dt > 0.f && std::fabs(dpx) < 200.f && std::fabs(dpy) < 200.f) {
            m_spd += (dpx / dt - m_spd) * 0.2f;
            m_vy += (dpy / dt - m_vy) * 0.2f;
        }
        m_lastPx = px;
        m_lastPy = py;
        if (moving) m_timer += dt;
        if (dt > 0.f) m_fps += (1.f / dt - m_fps) * 0.05f;

        // auto checkpoints: practice mode only
        if (pl && jg::flag("auto-checkpoints") && jg::isPractice(pl)) {
            if (moving) m_cpAcc += dt;
            if (m_cpAcc >= (float)jg::fnum("checkpoint-timer")) {
                m_cpAcc = 0.f;
                jg::tryCheckpoint(pl);
            }
        } else {
            m_cpAcc = 0.f;
        }

        // clicks per second over the last second
        double now = jg::nowSec();
        auto& q = jg::state.clickTimes;
        while (!q.empty() && now - q.front() > 1.0) q.pop_front();
        jg::state.cps = (int)q.size();

        this->drawProgress(pl);
        this->drawInput();

        m_acc += dt;
        if (m_acc < 0.1f) return;
        m_acc = 0.f;

        std::string t;
        auto add = [&](std::string const& s) {
            if (!t.empty()) t += "\n";
            t += s;
        };
        if (m_p && pl) add(fmt::format("{:.0f}%", pl->getCurrentPercent()));
        if (m_t) {
            int ts = (int)m_timer;
            add(fmt::format("{}:{:02}", ts / 60, ts % 60));
        }
        if (m_f) add(fmt::format("{:.0f} FPS", m_fps));
        if (m_xy && has) {
            add(fmt::format("X {:.0f}  Y {:.0f}", px, py));
            add(fmt::format("Rot {:.0f}", pl->m_player1->getRotation()));
            add(fmt::format("Speed {:.0f}  Vy {:+.0f}", m_spd, m_vy));
        }
        if (m_mode && has) {
            add(fmt::format("{}  Gravity {}", jg::modeName(pl->m_player1),
                            jg::gravityUp(pl->m_player1) ? "Up" : "Down"));
        }
        if (m_c) {
            std::time_t tt = std::time(nullptr);
            char buf[16] = {0};
            if (auto tmv = std::localtime(&tt)) std::strftime(buf, sizeof buf, "%H:%M", tmv);
            add(buf);
        }
        if (m_s) add(fmt::format("Attempt {}", jg::state.attempts));
        if (m_b) add(fmt::format("Best {}%", jg::state.best));
        if (m_k) add(fmt::format("Clicks {}", jg::state.clicks));
        if (m_cp) add(fmt::format("CPS {}", jg::state.cps));

        if (t != m_last) {
            m_last = t;
            m_l->setString(t.c_str());
        }
        m_l->setVisible(!t.empty());
    }

    static inline InfoHud* s_inst = nullptr;
    CCLabelBMFont* m_l = nullptr;
    CCLabelBMFont* m_il = nullptr;
    CCDrawNode* m_bar = nullptr;
    std::string m_last = "\x01", m_ilText;
    float m_fps = 60.f, m_acc = 1.f, m_lastPx = 0.f, m_lastPy = 0.f, m_timer = 0.f, m_cpAcc = 0.f;
    float m_spd = 0.f, m_vy = 0.f;
    bool m_p = false, m_f = false, m_t = false, m_c = false, m_s = false, m_b = false;
    bool m_k = false, m_cp = false, m_prog = false, m_mark = false;
    bool m_xy = false, m_mode = false, m_input = false;
};
