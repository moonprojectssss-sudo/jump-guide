#include "Menu.hpp"

JGMenu* JGMenu::create(GJGameLevel* level, bool inPause) {
    auto r = new JGMenu();
    if (r && r->setup(level, inPause)) {
        r->autorelease();
        return r;
    }
    CC_SAFE_DELETE(r);
    return nullptr;
}

bool JGMenu::setup(GJGameLevel* level, bool inPause) {
    if (!CCLayer::init()) return false;
    m_level = level;
    m_inPause = inPause;
    m_win = CCDirector::get()->getWinSize();
    m_baseScale = std::min(1.f, std::min(m_win.height * 0.94f / PH, m_win.width * 0.96f / PW));
    this->setTouchEnabled(true);

    m_gw = CCDrawNode::create();
    this->addChild(m_gw, 0);

    m_root = CCNode::create();
    m_root->setContentSize(CCSize(PW, PH));
    m_root->setAnchorPoint(CCPoint(0.5f, 0.5f));
    m_root->setPosition(CCPoint(m_win.width * 0.5f, m_win.height * 0.5f));
    m_root->setVisible(false);
    this->addChild(m_root, 1);

    m_gp = CCDrawNode::create();
    m_root->addChild(m_gp, 0);

    m_bubLab = CCLabelBMFont::create("AM", "bigFont.fnt");
    this->addChild(m_bubLab, 2);

    m_banLab = CCLabelBMFont::create("Tap an object to inspect it", "bigFont.fnt");
    m_banLab->setScale(0.3f);
    m_banLab->setAnchorPoint(CCPoint(0.f, 0.5f));
    m_banLab->setVisible(false);
    this->addChild(m_banLab, 2);
    m_banBtn = CCLabelBMFont::create("Cancel", "bigFont.fnt");
    m_banBtn->setScale(0.26f);
    m_banBtn->setVisible(false);
    this->addChild(m_banBtn, 2);

    auto* mod = Mod::get();
    m_bub = CCPoint((float)mod->getSavedValue<double>("bub-x", m_win.width - 34.0),
                    (float)mod->getSavedValue<double>("bub-y", m_win.height * 0.5));
    m_tabY = tabY(0);
    this->scheduleUpdate();
    return true;
}

std::string JGMenu::levelName() const {
    if (m_inPause) {
        if (auto pl = PlayLayer::get()) {
            if (pl->m_level) return std::string(pl->m_level->m_levelName);
        }
    }
    if (m_level) return std::string(m_level->m_levelName);
    return {};
}

void JGMenu::update(float dt) {
    m_dt = std::min(dt, 0.05f);
    m_time += m_dt;
    float k = std::min(1.f, m_dt * 13.f);
    float openGoal = m_open ? 1.f : 0.f;
    m_openT += (openGoal - m_openT) * k;
    if (std::fabs(openGoal - m_openT) < 0.004f) m_openT = openGoal;
    m_bubT += (((m_open || m_picking) ? 0.f : 1.f) - m_bubT) * k;
    m_pop = std::max(0.f, m_pop - m_dt * 4.f);

    this->drawWorld();

    bool show = m_openT > 0.01f;
    m_root->setVisible(show);
    if (show) {
        float us = jg::inum("ui-scale") / 100.f;
        float s = 0.86f + 0.14f * jg::easeOutBack(std::min(1.f, m_openT));
        m_root->setScale(m_baseScale * us * s);
        this->drawPanel();
    }

    // secret egg overlay
    float eggGoal = m_eggOpen ? 1.f : 0.f;
    m_eggOpenT += (eggGoal - m_eggOpenT) * k;
    if (std::fabs(eggGoal - m_eggOpenT) < 0.004f) m_eggOpenT = eggGoal;
    if (m_eggNode) {
        bool eggShow = m_eggOpenT > 0.01f;
        m_eggNode->setVisible(eggShow);
        if (eggShow) this->drawEgg();
    }
}

// Dim backdrop + the small round logo
void JGMenu::drawWorld() {
    m_gw->clear();
    m_banLab->setVisible(m_picking);
    m_banBtn->setVisible(m_picking);
    if (m_picking) { // slim banner while waiting for a tap on an object
        float bw = 360.f, bx0 = m_win.width * 0.5f - bw * 0.5f, by0 = m_win.height - 42.f;
        jg::roundRect(m_gw, bx0, by0, bx0 + bw, by0 + 30.f, 12.f,
                      jg::rgba(0.06f, 0.07f, 0.06f, 0.9f), 1.f, jg::rgba(1.f, 1.f, 1.f, 0.15f));
        m_banLab->setPosition(CCPoint(bx0 + 14.f, by0 + 15.f));
        float cx0 = bx0 + bw - 78.f;
        jg::roundRect(m_gw, cx0, by0 + 5.f, cx0 + 68.f, by0 + 25.f, 10.f, jg::rgba(1.f, 1.f, 1.f, 0.95f));
        m_banBtn->setPosition(CCPoint(cx0 + 34.f, by0 + 15.f));
        m_banBtn->setColor(jg::c3(0.1f, 0.1f, 0.1f));
        m_cancelRect = CCRect(cx0, by0, 68.f, 30.f);
    }
    if (m_openT > 0.01f) {
        jg::roundRect(m_gw, 0.f, 0.f, m_win.width, m_win.height, 0.01f,
                      jg::rgba(0.f, 0.f, 0.f, 0.55f * m_openT));
    }
    float bt = m_bubT;
    bool vis = bt > 0.01f;
    m_bubLab->setVisible(vis);
    if (!vis) return;

    bool calm = jg::flag("reduce-effects");
    float pulse = calm ? 0.f : 0.5f + 0.5f * std::sin(m_time * 2.4f);
    float us = jg::inum("ui-scale") / 100.f;
    float r = (20.f + (calm ? 0.f : m_pop * 5.f) + (m_drag ? 3.f : 0.f)) * us *
              jg::easeOutBack(std::min(1.f, bt));
    CCPoint c = m_bub;

    m_gw->drawDot(CCPoint(c.x + 1.5f, c.y - 2.5f), r + 1.f, jg::rgba(0.f, 0.f, 0.f, 0.28f * bt));
    m_gw->drawDot(c, r + 6.f + pulse * 3.f, jg::rgba(1.f, 1.f, 1.f, 0.10f * bt));
    m_gw->drawDot(c, r + 1.5f, jg::rgba(0.10f, 0.11f, 0.10f, bt));
    m_gw->drawDot(c, r - 1.5f, jg::rgba(0.97f, 0.97f, 0.98f, bt));

    m_bubLab->setPosition(c);
    m_bubLab->setScale(0.42f * r / 20.f);
    m_bubLab->setColor(jg::c3(0.10f, 0.11f, 0.10f));
    m_bubLab->setOpacity((GLubyte)(255.f * std::min(1.f, bt)));
}

void JGMenu::registerWithTouchDispatcher() {
    CCDirector::get()->getTouchDispatcher()->addTargetedDelegate(this, -510, true);
}

bool JGMenu::ccTouchBegan(CCTouch* t, CCEvent*) {
    if (m_eggOpen) return true; // the egg is modal
    if (m_picking) return true; // waiting for a tap on an object
    if (m_open) { // modal: the panel swallows every touch
        auto lp = m_root->convertToNodeSpace(t->getLocation());
        m_pressId.clear();
        m_slideId.clear();
        m_scrolling = false;
        m_touchStart = lp;
        m_lastY = lp.y;
        m_inContent = lp.x >= kX0 - 6.f && lp.x <= kX1 + 10.f && lp.y >= kBot && lp.y <= kTop;
        for (auto& h : m_hits) {
            if (!h.r.containsPoint(lp)) continue;
            m_pressId = h.id;
            if (h.slider) {
                m_slideId = h.id;
                if (h.sfn) h.sfn(std::clamp((lp.x - h.sx0) / (h.sx1 - h.sx0), 0.f, 1.f));
            }
            break;
        }
        return true;
    }
    auto p = this->convertToNodeSpace(t->getLocation());
    float dx = p.x - m_bub.x, dy = p.y - m_bub.y;
    if (dx * dx + dy * dy > 26.f * 26.f) return false; // let the game handle it
    m_drag = true;
    m_moved = false;
    m_dragOff = CCPoint(m_bub.x - p.x, m_bub.y - p.y);
    m_startP = p;
    return true;
}

void JGMenu::ccTouchMoved(CCTouch* t, CCEvent*) {
    if (m_eggOpen) return;
    if (m_picking) return;
    if (m_open) {
        auto lp = m_root->convertToNodeSpace(t->getLocation());
        if (!m_slideId.empty()) { // dragging a slider
            for (auto& h : m_hits) {
                if (h.id == m_slideId && h.slider && h.sfn) {
                    h.sfn(std::clamp((lp.x - h.sx0) / (h.sx1 - h.sx0), 0.f, 1.f));
                    break;
                }
            }
            return;
        }
        if (m_inContent) { // drag the list to scroll
            if (!m_scrolling && std::fabs(lp.y - m_touchStart.y) > 8.f) {
                m_scrolling = true;
                m_pressId.clear();
            }
            if (m_scrolling) m_scroll = std::clamp(m_scroll + (lp.y - m_lastY), 0.f, m_maxScroll);
        }
        m_lastY = lp.y;
        return;
    }
    if (!m_drag) return;
    // only the logo can be dragged, the panel stays put
    auto p = this->convertToNodeSpace(t->getLocation());
    if (std::fabs(p.x - m_startP.x) + std::fabs(p.y - m_startP.y) > 8.f) m_moved = true;
    if (!m_moved) return;
    m_bub.x = std::clamp(p.x + m_dragOff.x, 24.f, m_win.width - 24.f);
    m_bub.y = std::clamp(p.y + m_dragOff.y, 24.f, m_win.height - 24.f);
}

void JGMenu::ccTouchEnded(CCTouch* t, CCEvent*) {
    if (m_eggOpen) {
        if (!m_eggNode) { m_eggOpen = false; return; }
        auto ep = m_eggNode->convertToNodeSpace(t->getLocation());
        if (m_eggClose.containsPoint(ep) || !CCRect(0.f, 0.f, EW, EH).containsPoint(ep)) {
            this->closeEgg();
            return;
        }
        if (m_eggHam.containsPoint(ep)) {
            m_eggPop = 1.f;
            int n = Mod::get()->getSavedValue<int>("egg-kuaci", 0) + 1;
            Mod::get()->setSavedValue<int>("egg-kuaci", n);
        }
        return;
    }
    if (m_picking) {
        auto sp = this->convertToNodeSpace(t->getLocation());
        if (m_cancelRect.containsPoint(sp)) { this->stopPick(); return; }
        auto pl = PlayLayer::get();
        auto ol = jg::objectLayerOf(pl);
        if (pl && ol && jg::pickAt(pl, ol->convertToNodeSpace(t->getLocation()))) {
            m_pickMsg = "Picked. Check the details below.";
            this->stopPick();
            m_tab = 3;
            m_scroll = 0.f;
        } else {
            m_banLab->setString("Nothing there. Tap closer to an object");
        }
        return;
    }
    if (m_drag) {
        m_drag = false;
        if (m_moved) {
            Mod::get()->setSavedValue<double>("bub-x", m_bub.x);
            Mod::get()->setSavedValue<double>("bub-y", m_bub.y);
        } else {
            this->openMenu();
        }
        return;
    }
    if (!m_open) return;
    if (!m_slideId.empty()) {
        m_slideId.clear();
        m_pressId.clear();
        return;
    }
    if (m_scrolling) {
        m_scrolling = false;
        m_pressId.clear();
        return;
    }
    if (m_pressId.empty()) return;
    auto lp = m_root->convertToNodeSpace(t->getLocation());
    std::string id = m_pressId;
    m_pressId.clear();
    for (auto& h : m_hits) {
        if (h.id == id && h.r.containsPoint(lp)) {
            auto fn = h.fn;
            if (fn) fn();
            break;
        }
    }
}

void JGMenu::ccTouchCancelled(CCTouch*, CCEvent*) {
    m_drag = false;
    m_scrolling = false;
    m_slideId.clear();
    m_pressId.clear();
}

void JGMenu::openMenu() {
    m_open = true;
    m_pop = 1.f;
}

void JGMenu::closeMenu() {
    m_open = false;
    m_pressId.clear();
    m_slideId.clear();
    jg::applyLive();
}

// Reads the level directly: no copy and paste needed
void JGMenu::readLevel() {
    std::string data;
    if (m_inPause && PlayLayer::get()) data = jg::levelStringFromPlay(PlayLayer::get());
    else if (m_level) data = jg::levelStringFromLevel(m_level);

    std::string name = this->levelName();
    if (data.empty() || name.empty()) {
        m_status = "Could not read. Download the level first.";
        return;
    }
    jg::Stats st;
    auto evts = jg::buildEvents(data, &st);
    jg::store(name, evts);
    jg::state.name = name;
    jg::state.stats = st;
    jg::state.segs = evts.size();
    jg::state.has = true;
    m_status = evts.empty() ? std::string("No spikes or hold blocks found")
                            : fmt::format("{} segments ready", evts.size());
    if (auto bar = TimingBar::get()) bar->loadFor(name);
}

void JGMenu::resetAll() {
    auto* m = Mod::get();
    m->setSettingValue<bool>("enabled", true);
    m->setSettingValue<std::string>("side", "Right");
    m->setSettingValue<int64_t>("margin", 10);
    m->setSettingValue<double>("bar-height", 70.0);
    m->setSettingValue<int64_t>("bar-width", 84);
    m->setSettingValue<std::string>("bar-style", "Ladder");
    m->setSettingValue<double>("opacity", 100.0);
    m->setSettingValue<std::string>("accent", "Cyan");
    m->setSettingValue<double>("view-distance", 14.0);
    m->setSettingValue<int64_t>("press-offset", 90);
    m->setSettingValue<bool>("glow", true);
    m->setSettingValue<bool>("scroll-grid", true);
    m->setSettingValue<bool>("upright-only", true);
    for (char const* k : {"hud-percent", "hud-fps", "hud-timer", "hud-clock", "hud-session",
                          "hud-best", "hud-clicks", "hud-cps", "hud-progress", "hud-bestmark",
                          "auto-checkpoints", "death-markers", "click-sound", "high-contrast",
                          "large-text", "reduce-effects", "hitboxes", "block-grid", "trail",
                          "portal-labels", "start-markers", "input-display", "hud-coords",
                          "hud-mode"}) {
        m->setSettingValue<bool>(k, false);
    }
    m->setSettingValue<std::string>("hud-pos", "Top left");
    m->setSettingValue<double>("hud-size", 1.0);
    m->setSettingValue<double>("checkpoint-timer", 2.0);
    m->setSettingValue<int64_t>("music-volume", 100);
    m->setSettingValue<int64_t>("sfx-volume", 100);
    m->setSettingValue<std::string>("colorblind", "Off");
    m->setSettingValue<int64_t>("ui-scale", 100);
    m->setSettingValue<double>("trail-seconds", 3.0);
    m->setSettingValue<bool>("show-menu", true);
    jg::applyLive();
}

// Inspector: hide the pause screen so the level is visible, then wait for a tap
void JGMenu::startPick() {
    if (!(m_inPause && PlayLayer::get())) {
        m_pickMsg = "Open this from the pause screen.";
        return;
    }
    m_open = false;
    m_picking = true;
    jg::state.picking = true;
    m_pressId.clear();
    m_hidden.clear();
    if (auto par = this->getParent()) {
        if (auto kids = par->getChildren()) {
            for (auto c : CCArrayExt<CCNode*>(kids)) {
                if (!c || c == this) continue;
                m_hidden.push_back({c, c->isVisible()});
                c->setVisible(false);
            }
        }
    }
}

void JGMenu::stopPick() {
    m_picking = false;
    jg::state.picking = false;
    for (auto& [n, v] : m_hidden) n->setVisible(v);
    m_hidden.clear();
    m_banLab->setString("Tap an object to inspect it");
    this->openMenu();
}

// ---------------- secret egg: a hamster eating sunflower seeds ----------------

void JGMenu::eggTap() {
    if (m_time - m_eggLast > 1.2f) m_eggTaps = 0;
    m_eggLast = m_time;
    if (++m_eggTaps >= 7) {
        m_eggTaps = 0;
        this->buildEgg();
        if (m_eggNode) {
            m_eggOpen = true;
            m_eggPop = 1.f;
        }
    }
}

void JGMenu::closeEgg() {
    m_eggOpen = false;
}

void JGMenu::buildEgg() {
    if (m_eggNode) return;
    m_eggNode = CCNode::create();
    m_eggNode->setContentSize(CCSize(EW, EH));
    m_eggNode->setAnchorPoint(CCPoint(0.5f, 0.5f));
    m_eggNode->setPosition(CCPoint(m_win.width * 0.5f, m_win.height * 0.5f));
    m_eggNode->setVisible(false);
    this->addChild(m_eggNode, 3);

    m_ge = CCDrawNode::create();
    m_eggNode->addChild(m_ge, 0);

    std::vector<std::string> paths = {
        std::string("hamster_00.png"_spr), std::string("hamster_01.png"_spr),
        std::string("hamster_02.png"_spr), std::string("hamster_03.png"_spr),
        std::string("hamster_04.png"_spr), std::string("hamster_05.png"_spr),
        std::string("hamster_06.png"_spr), std::string("hamster_07.png"_spr),
        std::string("hamster_08.png"_spr), std::string("hamster_09.png"_spr),
        std::string("hamster_10.png"_spr), std::string("hamster_11.png"_spr),
        std::string("hamster_12.png"_spr), std::string("hamster_13.png"_spr),
        std::string("hamster_14.png"_spr)};
    for (auto& p : paths) {
        auto s = CCSprite::create(p.c_str());
        if (!s) continue;
        if (auto tex = s->getTexture()) tex->setAliasTexParameters(); // keep the pixel art crisp
        s->setPosition(CCPoint(EW * 0.5f, 146.f));
        s->setVisible(false);
        m_eggNode->addChild(s, 1);
        m_eggFrames.push_back(s);
    }

    m_eggTitle = CCLabelBMFont::create("Hamster Kuaci", "bigFont.fnt");
    m_eggTitle->setScale(0.36f);
    m_eggTitle->setPosition(CCPoint(EW * 0.5f - 6.f, EH - 24.f));
    m_eggTitle->setColor(jg::c3(0.10f, 0.10f, 0.10f));
    m_eggNode->addChild(m_eggTitle, 2);

    m_eggCount = CCLabelBMFont::create("", "bigFont.fnt");
    m_eggCount->setScale(0.3f);
    m_eggCount->setPosition(CCPoint(EW * 0.5f, 44.f));
    m_eggCount->setColor(jg::c3(0.10f, 0.10f, 0.10f));
    m_eggNode->addChild(m_eggCount, 2);

    m_eggHint = CCLabelBMFont::create(m_eggFrames.empty() ? "(images missing)" : "Tap the hamster!", "chatFont.fnt");
    m_eggHint->setScale(0.7f);
    m_eggHint->setPosition(CCPoint(EW * 0.5f, 20.f));
    m_eggHint->setColor(jg::c3(0.45f, 0.45f, 0.48f));
    m_eggNode->addChild(m_eggHint, 2);

    m_eggHam = CCRect(EW * 0.5f - 68.f, 70.f, 136.f, 152.f);
    m_eggClose = CCRect(EW - 38.f, EH - 40.f, 30.f, 30.f);
}

void JGMenu::drawEgg() {
    float a = std::min(1.f, m_eggOpenT);
    m_eggPop = std::max(0.f, m_eggPop - m_dt * 2.5f);
    m_eggT += m_dt * (1.f + m_eggPop * 1.5f);
    m_eggNode->setScale(m_baseScale * (0.85f + 0.15f * jg::easeOutBack(a)));

    m_ge->clear();
    jg::roundRect(m_ge, -4.f, -7.f, EW + 4.f, EH + 1.f, 22.f, jg::rgba(0.f, 0.f, 0.f, 0.20f * a));
    jg::roundRect(m_ge, 0.f, 0.f, EW, EH, 20.f, jg::rgba(0.97f, 0.97f, 0.98f, a), 1.6f,
                  jg::rgba(0.10f, 0.10f, 0.10f, a));
    m_ge->drawDot(CCPoint(EW - 23.f, EH - 25.f), 11.f, jg::rgba(0.10f, 0.10f, 0.10f, a));
    m_ge->drawSegment(CCPoint(EW - 27.f, EH - 29.f), CCPoint(EW - 19.f, EH - 21.f), 1.3f, jg::rgba(1.f, 1.f, 1.f, a));
    m_ge->drawSegment(CCPoint(EW - 27.f, EH - 21.f), CCPoint(EW - 19.f, EH - 29.f), 1.3f, jg::rgba(1.f, 1.f, 1.f, a));

    if (!m_eggFrames.empty()) {
        int n = (int)m_eggFrames.size();
        int f = (int)(m_eggT * 10.f) % n; // 10 frames per second
        if (f != m_eggFrame) {
            if (m_eggFrame >= 0 && m_eggFrame < n) m_eggFrames[m_eggFrame]->setVisible(false);
            m_eggFrames[f]->setVisible(true);
            m_eggFrame = f;
        }
        auto s = m_eggFrames[f];
        s->setScale(1.f + m_eggPop * 0.10f);
        s->setPosition(CCPoint(EW * 0.5f, 146.f + std::sin(m_eggPop * 3.14159f) * 10.f));
        s->setOpacity((GLubyte)(255.f * a));
    }

    int kuaci = Mod::get()->getSavedValue<int>("egg-kuaci", 0);
    if (kuaci != m_eggShown) {
        m_eggShown = kuaci;
        m_eggCount->setString(fmt::format("Kuaci eaten: {}", kuaci).c_str());
    }
    GLubyte op = (GLubyte)(255.f * a);
    m_eggTitle->setOpacity(op);
    m_eggCount->setOpacity(op);
    m_eggHint->setOpacity(op);
}
