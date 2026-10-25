#pragma once
#include "Hud.hpp"
#include <functional>
#include <map>
#include <set>

namespace jg {
// Re-reads settings in every live widget (bar, overlay)
inline void applyLive() {
    if (auto b = TimingBar::get()) b->applySettings();
    if (auto h = InfoHud::get()) h->applySettings();
}
// Called whenever a setting is changed from the menu
inline void onChanged(std::string const& key) {
    if (key == "music-volume") setMusicVolume(inum("music-volume") / 100.f);
    else if (key == "sfx-volume") setSfxVolume(inum("sfx-volume") / 100.f);
    applyLive();
}
}

// Small draggable round logo. Tap it to open the Assist Menu panel.
// Only added to the pause menu and the level page, never inside a level.
class JGMenu : public CCLayer {
public:
    static JGMenu* create(GJGameLevel* level, bool inPause);

protected:
    struct Hit {
        std::string id;
        CCRect r;
        std::function<void()> fn;
        bool slider = false;
        float sx0 = 0.f, sx1 = 0.f;
        std::function<void(float)> sfn;
    };
    struct Lab { CCLabelBMFont* n = nullptr; std::string s; };

    static constexpr float PW = 410.f, PH = 300.f;
    static constexpr float kX0 = 92.f, kX1 = 396.f, kRH = 26.f;
    static constexpr float kTop = PH - 48.f, kBot = 10.f;
    static float tabY(int i) { return PH - 68.f - 34.f * i; }
    float rowY(int i) const { return kTop - kRH * (i + 1) + m_scroll; }
    bool vis(float y, float h = kRH) const { return y >= kBot - 0.5f && y + h <= kTop + 0.5f; }

    // core (Menu.cpp)
    bool setup(GJGameLevel* level, bool inPause);
    void update(float dt) override;
    void registerWithTouchDispatcher() override;
    bool ccTouchBegan(CCTouch* t, CCEvent* e) override;
    void ccTouchMoved(CCTouch* t, CCEvent* e) override;
    void ccTouchEnded(CCTouch* t, CCEvent* e) override;
    void ccTouchCancelled(CCTouch* t, CCEvent* e) override;
    void drawWorld();
    void openMenu();
    void closeMenu();
    void readLevel();
    void resetAll();
    std::string levelName() const;

    // panel (MenuPanel.cpp)
    void drawPanel();
    void tabContent();
    ccColor4F A(ccColor4F c, float a = 1.f) const {
        return ccColor4F{c.r, c.g, c.b, c.a * a * m_openT};
    }
    float anim(std::string const& id, float target, float speed);
    void lab(std::string const& id, std::string const& txt, bool big, float x, float y,
             float sc, ccColor3B col, float ax);
    void addHit(std::string const& id, float x0, float y0, float x1, float y1,
                std::function<void()> fn);
    void rowBase(std::string const& id, std::string const& label, float y);
    void rowHead(std::string const& id, std::string const& txt, float y);
    void rowText(std::string const& id, std::string const& txt, float y);
    void rowInfo(std::string const& id, std::string const& label, std::string const& value, float y);
    void rowSwitch(std::string const& id, std::string const& label, std::string const& key, float y);
    void rowSlider(std::string const& id, std::string const& label, std::string const& key,
                   bool isInt, double lo, double hi, double step, int dec,
                   std::string const& suffix, float y);
    void rowCycle(std::string const& id, std::string const& label, std::string const& key,
                  std::vector<std::string> const& opts, float y);
    void rowAction(std::string const& id, std::string const& label, std::string const& btn,
                   float y, std::function<void()> fn);
    void chartHeat(float y);
    void chartGraph(float y);

    GJGameLevel* m_level = nullptr;
    bool m_inPause = false;
    CCSize m_win;
    CCDrawNode* m_gw = nullptr; // backdrop + round logo
    CCDrawNode* m_gp = nullptr; // panel
    CCNode* m_root = nullptr;
    CCLabelBMFont* m_bubLab = nullptr;
    std::map<std::string, Lab> m_labs;
    std::set<std::string> m_used;
    std::map<std::string, float> m_anim;
    std::vector<Hit> m_hits;
    std::string m_pressId, m_slideId, m_status;
    CCPoint m_bub, m_dragOff, m_startP, m_touchStart;
    bool m_open = false, m_drag = false, m_moved = false;
    bool m_scrolling = false, m_inContent = false;
    float m_openT = 0.f, m_bubT = 0.f, m_time = 0.f, m_dt = 0.016f;
    float m_tabY = 232.f, m_pop = 0.f, m_baseScale = 1.f, m_lastY = 0.f;
    float m_scroll = 0.f, m_maxScroll = 0.f, m_contentH = 0.f;
    int m_tab = 0;
};
