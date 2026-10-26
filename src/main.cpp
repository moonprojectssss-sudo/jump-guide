#include <Geode/modify/EditorUI.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/ui/Notification.hpp>
#include "Menu.hpp"

// Inside a level: timing bar, info overlay, run history. The menu is NOT added here.
class $modify(JGPlayLayer, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
        jg::state.resetSession();
        jg::resetLevelState();
        jg::state.markerPts.clear();
        jg::state.markerNode = nullptr;

        if (auto bar = TimingBar::create()) {
            bar->loadFor(std::string(level->m_levelName));
            m_uiLayer->addChild(bar, 50);
        }
        if (auto hud = InfoHud::create()) m_uiLayer->addChild(hud, 49);

        // layer for the "death markers" (if this game version exposes the object layer)
        if (auto ol = jg::objectLayerOf(this)) {
            auto n = CCDrawNode::create();
            ol->addChild(n, 1000);
            jg::state.markerNode = n;
            if (auto dv = DebugView::create()) ol->addChild(dv, 999);
        }
        return true;
    }

    void resetLevel() {
        int reached = this->getCurrentPercentInt(); // where the attempt that just ended got to
        CCPoint where = m_player1 ? m_player1->getPosition() : CCPoint(0.f, 0.f);
        PlayLayer::resetLevel();

        if (jg::state.armed) { // skip the very first reset when the level loads
            jg::recordRun(reached);
            jg::state.best = std::max(jg::state.best, reached);
            if (jg::flag("death-markers") && reached > 0 && reached < 100) jg::addMarker(where);
        }
        jg::state.armed = true;
        jg::state.attempts++;
    }
};

// Pause menu: the round logo appears here
class $modify(JGPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();
        if (!jg::flag("show-menu")) return;
        GJGameLevel* lvl = nullptr;
        if (auto pl = PlayLayer::get()) lvl = pl->m_level;
        if (auto menu = JGMenu::create(lvl, true)) this->addChild(menu, 100);
    }
};

// Level page (before playing): the round logo appears here too
class $modify(JGInfoLayer, LevelInfoLayer) {
    bool init(GJGameLevel* level, bool challenge) {
        if (!LevelInfoLayer::init(level, challenge)) return false;
        if (jg::flag("show-menu")) {
            if (auto menu = JGMenu::create(level, false)) this->addChild(menu, 100);
        }
        return true;
    }
};

// Jump button: feeds the timing bar and the click counter
class $modify(JGBaseLayer, GJBaseGameLayer) {
    void handleButton(bool down, int button, bool isPlayer1) {
        GJBaseGameLayer::handleButton(down, button, isPlayer1);
        if (button != 1) return;
        jg::state.down = down;
        if (down) {
            jg::state.downSince = jg::nowSec();
            jg::state.clicks++;
            jg::state.clickTimes.push_back(jg::nowSec());
            if (jg::flag("click-sound")) jg::playClick();
        }
        if (auto bar = TimingBar::get()) bar->onButton(down);
    }
};

// Editor: pressing Copy still saves the selection as segments (optional way)
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
        Notification::create(fmt::format("Assist Menu: {} segments saved", evts.size()),
                             NotificationIcon::Success, 1.2f)->show();
    }
};
