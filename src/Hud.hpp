#pragma once
#include "TimingBar.hpp"
#include <ctime>

// Read-only info overlay shown while playing. Nothing here changes gameplay.
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
        m_c = jg::flag("hud-clock");
        m_s = jg::flag("hud-session");
        m_b = jg::flag("hud-best");
        m_acc = 1.f; // force a refresh on the next frame
    }

protected:
    bool init() override {
        if (!CCNode::init()) return false;
        auto win = CCDirector::get()->getWinSize();
        m_l = CCLabelBMFont::create("", "bigFont.fnt");
        m_l->setAnchorPoint({0.f, 1.f});
        m_l->setScale(0.32f);
        m_l->setOpacity(210);
        m_l->setPosition({18.f, win.height - 14.f});
        this->addChild(m_l);
        this->applySettings();
        this->scheduleUpdate();
        s_inst = this;
        return true;
    }

    void update(float dt) override {
        if (dt > 0.f) m_fps += (1.f / dt - m_fps) * 0.05f;
        m_acc += dt;
        if (m_acc < 0.2f) return;
        m_acc = 0.f;

        std::string t;
        auto add = [&](std::string const& s) {
            if (!t.empty()) t += "\n";
            t += s;
        };
        auto pl = PlayLayer::get();
        if (m_p && pl) add(fmt::format("{:.0f}%", pl->getCurrentPercent()));
        if (m_f) add(fmt::format("{:.0f} FPS", m_fps));
        if (m_c) {
            std::time_t now = std::time(nullptr);
            char buf[16] = {0};
            if (auto tmv = std::localtime(&now)) std::strftime(buf, sizeof buf, "%H:%M", tmv);
            add(buf);
        }
        if (m_s) add(fmt::format("Attempt {}", jg::state.attempts));
        if (m_b) add(fmt::format("Best {}%", jg::state.best));

        if (t != m_last) {
            m_last = t;
            m_l->setString(t.c_str());
        }
        m_l->setVisible(!t.empty());
    }

    static inline InfoHud* s_inst = nullptr;
    CCLabelBMFont* m_l = nullptr;
    std::string m_last = "\x01";
    float m_fps = 60.f, m_acc = 1.f;
    bool m_p = false, m_f = false, m_c = false, m_s = false, m_b = false;
};
