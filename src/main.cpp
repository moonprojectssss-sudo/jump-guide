#include <Geode/modify/EditorUI.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/ui/Notification.hpp>
#include "Menu.hpp"

// Inside a level: timing bar + info overlay. The menu is NOT added here.
class $modify(JGPlayLayer, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
        jg::state.attempts = 0;
        jg::state.best = 0;

        if (auto bar = TimingBar::create()) {
            bar->loadFor(std::string(level->m_levelName));
            m_uiLayer->addChild(bar, 50);
        }
        if (auto hud = InfoHud::create()) m_uiLayer->addChild(hud, 49);
        return true;
    }

    void resetLevel() {
        int reached = this->getCurrentPercentInt(); // where this attempt ended
        PlayLayer::resetLevel();
        jg::state.best = std::max(jg::state.best, reached);
        jg::state.attempts++;
    }
};

// Pause menu: round logo appears here
class $modify(JGPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();
        if (!jg::flag("show-menu")) return;
        GJGameLevel* lvl = nullptr;
        if (auto pl = PlayLayer::get()) lvl = pl->m_level;
        if (auto menu = JGMenu::create(lvl, true)) this->addChild(menu, 100);
    }
};

// Level page (before playing): round logo appears here too
class $modify(JGInfoLayer, LevelInfoLayer) {
    bool init(GJGameLevel* level, bool challenge) {
        if (!LevelInfoLayer::init(level, challenge)) return false;
        if (jg::flag("show-menu")) {
            if (auto menu = JGMenu::create(level, false)) this->addChild(menu, 100);
        }
        return true;
    }
};

// Jump button feeds the bar
class $modify(JGBaseLayer, GJBaseGameLayer) {
    void handleButton(bool down, int button, bool isPlayer1) {
        GJBaseGameLayer::handleButton(down, button, isPlayer1);
        if (button == 1) {
            if (auto bar = TimingBar::get()) bar->onButton(down);
        }
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
        Notification::create(fmt::format("Jump Guide: {} segments saved", evts.size()),
                             NotificationIcon::Success, 1.2f)->show();
    }
};
