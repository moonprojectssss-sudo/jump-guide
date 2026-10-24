#pragma once
#include "Hud.hpp"
#include <functional>
#include <map>
#include <set>

namespace jg {
// Re-reads settings in every live widget (bar, info overlay)
inline void applyLive() {
    if (auto b = TimingBar::get()) b->applySettings();
    if (auto h = InfoHud::get()) h->applySettings();
}
}

// Small draggable round logo. Tap it to open the menu panel.
// Only added to the pause menu and the level page, never inside a level.
class JGMenu : public CCLayer {
public:
    static JGMenu* create(GJGameLevel* level, bool inPause);

protected:
    struct Hit { std::string id; CCRect r; std::function<void()> fn; };
    struct Lab { CCLabelBMFont* n = nullptr; std::string s; };

    static constexpr float PW = 380.f, PH = 270.f;
    static constexpr float kX0 = 86.f, kX1 = 366.f, kRH = 22.f;
    static float tabY(int i) { return 190.f - i * 40.f; }
    static float rowY(int i) { return 202.f - i * 26.f; }

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
    void cardAt(float y);
    void rowText(std::string const& id, std::string const& txt, float y);
    void rowInfo(std::string const& id, std::string const& label, std::string const& value, float y);
    void rowCheck(std::string const& id, std::string const& label, std::string const& key, float y);
    void rowButton(std::string const& id, std::string const& label, float y, bool primary,
                   std::function<void()> fn);
    void rowStep(std::string const& id, std::string const& label, std::string const& key,
                 bool isInt, double lo, double hi, double step, std::string const& suffix, float y);
    void rowCycle(std::string const& id, std::string const& label, std::string const& key,
                  std::vector<std::string> const& opts, float y);

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
    std::string m_pressId, m_status;
    CCPoint m_bub, m_dragOff, m_startP;
    bool m_open = false, m_drag = false, m_moved = false;
    float m_openT = 0.f, m_bubT = 0.f, m_time = 0.f, m_dt = 0.016f;
    float m_tabY = 190.f, m_pop = 0.f, m_baseScale = 1.f;
    int m_tab = 0;
};
