#pragma once
#include "Draw.hpp"
#include <deque>

// Visual aids drawn inside the level (display only, nothing here changes gameplay):
// hitbox viewer, player trail, block grid, portal labels, start and checkpoint markers.
class DebugView : public CCNode {
public:
    static DebugView* create() {
        auto r = new DebugView();
        if (r && r->init()) {
            r->autorelease();
            return r;
        }
        CC_SAFE_DELETE(r);
        return nullptr;
    }
    static DebugView* get() { return s_inst; }

    ~DebugView() override {
        if (s_inst == this) s_inst = nullptr;
    }

    void applySettings() {
        m_hit = jg::flag("hitboxes");
        m_grid = jg::flag("block-grid");
        m_trailOn = jg::flag("trail");
        m_trailSec = std::max(0.5f, (float)jg::fnum("trail-seconds"));
        m_portal = jg::flag("portal-labels");
        m_start = jg::flag("start-markers");
        if (!m_trailOn) m_trail.clear();
    }

protected:
    struct TP { float t; CCPoint p; };

    bool init() override {
        if (!CCNode::init()) return false;
        m_g = CCDrawNode::create();
        this->addChild(m_g, 0);
        this->applySettings();
        this->scheduleUpdate();
        s_inst = this;
        return true;
    }

    void box(CCRect const& r, ccColor4F c, float w) {
        CCPoint v[4] = {CCPoint(r.getMinX(), r.getMinY()), CCPoint(r.getMaxX(), r.getMinY()),
                        CCPoint(r.getMaxX(), r.getMaxY()), CCPoint(r.getMinX(), r.getMaxY())};
        m_g->drawPolygon(v, 4, ccColor4F{c.r, c.g, c.b, 0.12f}, w, c);
    }

    void flagAt(float x, float y, ccColor4F c) {
        m_g->drawSegment(CCPoint(x, y - 12.f), CCPoint(x, y + 30.f), 1.2f, c);
        jg::tri(m_g, CCPoint(x, y + 30.f), CCPoint(x, y + 16.f), CCPoint(x + 14.f, y + 23.f), c);
    }

    void put(size_t& i, std::string const& s, float x, float y, ccColor3B col) {
        while (m_labels.size() <= i) {
            auto l = CCLabelBMFont::create("", "bigFont.fnt");
            l->setScale(0.22f);
            l->setOpacity(225);
            this->addChild(l, 1);
            m_labels.push_back(l);
            m_texts.push_back("");
        }
        auto l = m_labels[i];
        if (m_texts[i] != s) {
            l->setString(s.c_str());
            m_texts[i] = s;
        }
        l->setPosition(CCPoint(x, y));
        l->setColor(col);
        l->setVisible(true);
        ++i;
    }

    void update(float dt) override {
        auto pl = PlayLayer::get();
        if (!pl || !pl->m_player1) return;
        m_g->clear();
        for (auto l : m_labels) l->setVisible(false);

        auto win = CCDirector::get()->getWinSize();
        float px = pl->m_player1->getPositionX(), py = pl->m_player1->getPositionY();
        bool moved = std::fabs(px - m_cx) + std::fabs(py - m_cy) > 0.01f;
        m_cx = px;
        m_cy = py;
        if (moved) m_clock += dt;

        ccColor4F RED{1.00f, 0.25f, 0.30f, 1.f}, BLUE{0.30f, 0.55f, 1.00f, 1.f};
        ccColor4F GREEN{0.25f, 1.00f, 0.50f, 1.f}, AMBER{1.00f, 0.78f, 0.20f, 1.f};
        size_t li = 0;

        // checkpoints placed in practice mode (remember where the count grew)
        int cc = jg::checkpointCount(pl);
        if (cc >= 0) {
            auto& cps = jg::state.checkpoints;
            while ((int)cps.size() < cc) cps.push_back(CCPoint(px, py));
            while ((int)cps.size() > cc) cps.pop_back();
        }

        // trail
        if (px + 8.f < m_lastPx) m_trail.clear(); // respawn
        m_lastPx = px;
        if (m_trailOn) {
            if (m_trail.empty() || std::fabs(m_trail.back().p.x - px) + std::fabs(m_trail.back().p.y - py) > 0.4f) {
                m_trail.push_back(TP{m_clock, CCPoint(px, py)});
            }
            while (!m_trail.empty() && m_clock - m_trail.front().t > m_trailSec) m_trail.pop_front();
            for (size_t k = 1; k < m_trail.size(); ++k) {
                float a = 1.f - (m_clock - m_trail[k].t) / m_trailSec;
                m_g->drawSegment(m_trail[k - 1].p, m_trail[k].p, 1.4f,
                                 ccColor4F{0.35f, 1.f, 0.60f, std::clamp(a, 0.f, 1.f) * 0.9f});
            }
        }

        // block grid
        float x0 = px - 130.f, x1 = px + win.width + 70.f;
        if (m_grid) {
            float gx0 = std::floor((px - 150.f) / 30.f) * 30.f;
            float gy0 = std::floor((py - 170.f) / 30.f) * 30.f, gy1 = py + 230.f;
            for (float x = gx0; x <= x1; x += 30.f) {
                long n = std::lround(x / 30.f);
                bool major = n % 5 == 0;
                m_g->drawSegment(CCPoint(x, gy0), CCPoint(x, gy1), 0.5f,
                                 ccColor4F{1.f, 1.f, 1.f, major ? 0.22f : 0.09f});
                if (n % 10 == 0 && li < 8) put(li, fmt::format("{}", n), x + 2.f, gy0 + 10.f, jg::c3(0.8f, 0.8f, 0.8f));
            }
            for (float y = gy0; y <= gy1; y += 30.f) {
                long n = std::lround(y / 30.f);
                m_g->drawSegment(CCPoint(gx0, y), CCPoint(x1, y), 0.5f,
                                 ccColor4F{1.f, 1.f, 1.f, n % 5 == 0 ? 0.22f : 0.09f});
            }
        }

        // objects in view: hitboxes, portal labels, start positions
        bool needObjs = m_hit || m_portal || m_start || jg::state.pick.has;
        if (needObjs) jg::ensureCache(pl);
        auto& cache = jg::objCache;
        if (needObjs && !cache.empty()) {
            auto lo = std::lower_bound(cache.begin(), cache.end(), x0,
                                       [](jg::CObj const& c, float v) { return c.x < v; });
            int boxes = 0;
            for (auto it = lo; it != cache.end() && it->x <= x1; ++it) {
                int t = it->type;
                if (m_hit && boxes < 800 && (t == 2 || t == 0 || t == 25)) {
                    CCRect r;
                    if (jg::objRect(it->o, r)) {
                        box(r, t == 2 ? RED : BLUE, 1.1f);
                        ++boxes;
                    }
                }
                if (m_portal && li < 16) {
                    if (auto txt = jg::portalLabel(it->id)) {
                        put(li, txt, it->x, it->y + 34.f, jg::c3(1.f, 0.85f, 0.35f));
                    }
                }
                if (m_start && it->id == 31 && li < 20) {
                    flagAt(it->x, it->y, AMBER);
                    put(li, "Start", it->x + 4.f, it->y + 42.f, jg::c3(1.f, 0.85f, 0.35f));
                }
            }
        }

        // checkpoints
        if (m_start) {
            auto& cps = jg::state.checkpoints;
            for (size_t k = 0; k < cps.size(); ++k) {
                auto p = cps[k];
                if (p.x < x0 || p.x > x1) continue;
                m_g->drawSegment(CCPoint(p.x, p.y + 12.f), CCPoint(p.x + 9.f, p.y), 1.6f, GREEN);
                m_g->drawSegment(CCPoint(p.x + 9.f, p.y), CCPoint(p.x, p.y - 12.f), 1.6f, GREEN);
                m_g->drawSegment(CCPoint(p.x, p.y - 12.f), CCPoint(p.x - 9.f, p.y), 1.6f, GREEN);
                m_g->drawSegment(CCPoint(p.x - 9.f, p.y), CCPoint(p.x, p.y + 12.f), 1.6f, GREEN);
                if (li < 24) put(li, fmt::format("CP {}", k + 1), p.x, p.y + 24.f, jg::c3(0.4f, 1.f, 0.6f));
            }
        }

        // players
        if (m_hit) {
            CCRect pr;
            if (jg::objRect(pl->m_player1, pr)) box(pr, GREEN, 1.5f);
            if (pl->m_player2 && pl->m_player2->isVisible() && jg::objRect(pl->m_player2, pr)) box(pr, GREEN, 1.5f);
        }

        // object picked in the inspector
        if (jg::state.pick.has) {
            auto& k = jg::state.pick;
            box(CCRect(k.x - 17.f, k.y - 17.f, 34.f, 34.f), AMBER, 1.6f);
        }
    }

    static inline DebugView* s_inst = nullptr;
    CCDrawNode* m_g = nullptr;
    std::vector<CCLabelBMFont*> m_labels;
    std::vector<std::string> m_texts;
    std::deque<TP> m_trail;
    float m_clock = 0.f, m_trailSec = 3.f, m_lastPx = 0.f, m_cx = 0.f, m_cy = 0.f;
    bool m_hit = false, m_grid = false, m_trailOn = false, m_portal = false, m_start = false;
};
