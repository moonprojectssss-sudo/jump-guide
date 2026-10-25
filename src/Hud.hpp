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

    void update(float dt) override {
        auto pl = PlayLayer::get();
        float px = (pl && pl->m_player1) ? pl->m_player1->getPositionX() : 0.f;
        bool moving = std::fabs(px - m_lastPx) > 0.01f;
        if (px + 8.f < m_lastPx) { m_timer = 0.f; m_cpAcc = 0.f; } // new attempt
        m_lastPx = px;
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
    CCDrawNode* m_bar = nullptr;
    std::string m_last = "\x01";
    float m_fps = 60.f, m_acc = 1.f, m_lastPx = 0.f, m_timer = 0.f, m_cpAcc = 0.f;
    bool m_p = false, m_f = false, m_t = false, m_c = false, m_s = false, m_b = false;
    bool m_k = false, m_cp = false, m_prog = false, m_mark = false;
};
