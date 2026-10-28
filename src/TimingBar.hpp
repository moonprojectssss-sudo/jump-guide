#pragma once
#include "Data.hpp"
#include "Draw.hpp"

class TimingBar : public CCNode {
public:
    static TimingBar* create();
    static TimingBar* get() { return s_inst; }

    void loadFor(std::string const& levelName);
    void onButton(bool down);
    void applySettings();
    void resetStreak() { m_streak = 0; m_shown = -99; }
    // true while the bar is visible and has level data, i.e. it is really guiding you
    bool isAssisting() { return this->isVisible() && !m_events.empty(); }

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

    // ladder style: perspective track (TimingDraw.cpp)
    void drawStaticLadder();
    void drawFrameLadder(float lineX, float dt);
    void drawRung(jg::Evt const& e, float t0, float t1, float tmin);
    void drawLineLadder();
    void quadT(CCDrawNode* d, float ta, float tb, float minH, float inset, ccColor4F fill,
               float bw, ccColor4F border);
    float depthScale(float t) const;
    float depthY(float t) const;

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
    ccColor4F m_accent{}, m_good{}, m_bad{};

    float m_w = 0.f, m_h = 0.f, m_pad = 8.f, m_alpha = 1.f;
    float m_yBot = 10.f, m_yLine = 0.f, m_yTop = 0.f, m_pxU = 1.f;
    float m_viewUnits = 420.f, m_offsetSec = 0.09f, m_preSec = 0.12f;
    float m_lineX = 0.f, m_lastPx = 0.f, m_speed = 311.58f;
    float m_flash = 0.f, m_time = 0.f;
    bool m_down = false, m_pressed = false, m_grid = true, m_glowOn = true;
    bool m_hc = false, m_reduce = false, m_ladder = true;
    int m_streak = 0, m_shown = -99;
    size_t m_first = 0;
};
