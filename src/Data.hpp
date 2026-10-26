#pragma once
#include <Geode/Geode.hpp>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <string>
#include <vector>

using namespace geode::prelude;

namespace jg {

struct Span { float x0, x1; bool ok; };

// One segment on the timing bar (spike cluster = tap, marker run = hold)
struct Evt {
    float x0 = 0.f, x1 = 0.f;
    bool hold = false;
    float cursor = 0.f;
    bool hit = false, done = false;
    std::vector<Span> spans;
};

struct Stats { int objects = 0, spikes = 0, holds = 0; float maxX = 0.f; };

// Runtime state shared by the menu, the overlay and the bar
struct Shared {
    std::string name;
    Stats stats;
    size_t segs = 0;
    bool has = false;
    int attempts = 0, best = 0, clicks = 0, cps = 0;
    bool armed = false;
    std::deque<int> history;       // where the last 20 runs ended (percent)
    int bins[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // runs ended per 10%
    std::deque<double> clickTimes;
    CCDrawNode* markerNode = nullptr;
    std::vector<CCPoint> markerPts;

    // input display
    bool down = false;
    double downSince = 0.0;

    // checkpoints seen in practice mode
    std::vector<CCPoint> checkpoints;

    // object inspector
    struct Pick {
        bool has = false;
        int id = 0, c1 = 0, c2 = 0, type = -1;
        float x = 0.f, y = 0.f, rot = 0.f;
        bool flipY = false;
    } pick;
    bool picking = false;
    PlayLayer* cacheOwner = nullptr;

    void resetSession() {
        attempts = 0; best = 0; clicks = 0; cps = 0; armed = false;
        history.clear();
        clickTimes.clear();
        for (auto& b : bins) b = 0;
    }
};
inline Shared state;

inline double fnum(char const* k) { return Mod::get()->getSettingValue<double>(k); }
inline int inum(char const* k) { return (int)Mod::get()->getSettingValue<int64_t>(k); }
inline bool flag(char const* k) { return Mod::get()->getSettingValue<bool>(k); }
inline std::string text(char const* k) { return Mod::get()->getSettingValue<std::string>(k); }

inline double nowSec() {
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}

inline void recordRun(int pct) {
    pct = std::clamp(pct, 0, 100);
    state.history.push_back(pct);
    if (state.history.size() > 20) state.history.pop_front();
    state.bins[std::min(9, pct / 10)]++;
}

// Little red crosses where earlier attempts ended (display only)
inline void redrawMarkers() {
    auto n = state.markerNode;
    if (!n) return;
    n->clear();
    ccColor4F c{1.f, 0.30f, 0.35f, 0.9f};
    for (auto& p : state.markerPts) {
        n->drawSegment(CCPoint(p.x - 6.f, p.y - 6.f), CCPoint(p.x + 6.f, p.y + 6.f), 1.5f, c);
        n->drawSegment(CCPoint(p.x - 6.f, p.y + 6.f), CCPoint(p.x + 6.f, p.y - 6.f), 1.5f, c);
    }
}

inline void addMarker(CCPoint p) {
    state.markerPts.push_back(p);
    if (state.markerPts.size() > 60) state.markerPts.erase(state.markerPts.begin());
    redrawMarkers();
}

// ---- helpers that quietly do nothing when this game version lacks a member ----

template <class PL> inline bool isPractice(PL* pl) {
    if constexpr (requires { pl->m_isPracticeMode; }) return pl && pl->m_isPracticeMode;
    else return false;
}

template <class PL> inline bool tryCheckpoint(PL* pl) {
    if constexpr (requires { pl->createCheckpoint(); }) {
        if (!pl) return false;
        pl->createCheckpoint();
        return true;
    } else return false;
}

template <class PL> inline CCNode* objectLayerOf(PL* pl) {
    if constexpr (requires { pl->m_objectLayer; }) return pl->m_objectLayer;
    else return nullptr;
}

template <class E = FMODAudioEngine> inline E* audio() {
    if constexpr (requires { E::sharedEngine(); }) return E::sharedEngine();
    else return nullptr;
}

template <class E> inline void musicVolImpl(E* e, float v) {
    if constexpr (requires { e->setBackgroundMusicVolume(v); }) { if (e) e->setBackgroundMusicVolume(v); }
}
template <class E> inline void sfxVolImpl(E* e, float v) {
    if constexpr (requires { e->setEffectsVolume(v); }) { if (e) e->setEffectsVolume(v); }
}
template <class E> inline void clickImpl(E* e) {
    if constexpr (requires { e->playEffect(std::string("playSound_01.ogg"), 1.f, 1.f, 0.5f); }) {
        if (e) e->playEffect(std::string("playSound_01.ogg"), 1.f, 1.f, 0.5f);
    }
}
inline void setMusicVolume(float v) { musicVolImpl(audio(), v); }
inline void setSfxVolume(float v) { sfxVolImpl(audio(), v); }
inline void playClick() { clickImpl(audio()); }

inline float unitsPerSec(float s) {
    if (s < 0.8f) return 251.16f;
    if (s < 1.0f) return 311.58f;
    if (s < 1.2f) return 387.42f;
    if (s < 1.45f) return 468.0f;
    return 576.0f;
}

struct Obj { int id = 0, c1 = 0, c2 = 0; float x = 0.f, rot = 0.f; bool flipY = false; };

inline bool readObj(char const* p, char const* end, Obj& o) {
    int key = 0;
    bool isKey = true;
    while (p < end) {
        if (isKey) key = (int)std::strtol(p, nullptr, 10);
        else switch (key) {
            case 1:  o.id = (int)std::strtol(p, nullptr, 10); break;
            case 2:  o.x = std::strtof(p, nullptr); break;
            case 5:  o.flipY = std::strtol(p, nullptr, 10) != 0; break;
            case 6:  o.rot = std::strtof(p, nullptr); break;
            case 21: o.c1 = (int)std::strtol(p, nullptr, 10); break;
            case 22: o.c2 = (int)std::strtol(p, nullptr, 10); break;
            default: break;
        }
        auto c = (char const*)std::memchr(p, ',', end - p);
        if (!c) break;
        p = c + 1;
        isKey = !isKey;
    }
    return o.id != 0;
}

inline std::vector<int> parseIds(std::string const& s) {
    std::vector<int> v;
    char const* p = s.c_str();
    while (*p) {
        char* n = nullptr;
        long id = std::strtol(p, &n, 10);
        if (n == p) { ++p; continue; }
        v.push_back((int)id);
        p = n;
    }
    return v;
}

inline void mergeInto(std::vector<float> xs, float gap, bool hold, std::vector<Evt>& out) {
    if (xs.empty()) return;
    std::sort(xs.begin(), xs.end());
    float a = xs[0], b = xs[0];
    auto push = [&] {
        Evt e;
        e.hold = hold;
        e.x0 = a - 15.f;
        e.x1 = b + 15.f;
        e.cursor = e.x0;
        out.push_back(e);
    };
    for (size_t i = 1; i < xs.size(); ++i) {
        if (xs[i] - b <= gap) b = xs[i];
        else { push(); a = b = xs[i]; }
    }
    push();
}

// Level object string ("key,value,..;key,value,..;") -> sorted segments (+ optional stats)
inline std::vector<Evt> buildEvents(std::string const& s, Stats* st = nullptr) {
    auto ids = parseIds(text("danger-ids"));
    int channel = inum("hold-channel");
    bool upright = flag("upright-only");
    float gap = (float)fnum("merge-gap") * 30.f;
    std::vector<float> spikes, holds;

    char const* p = s.data();
    char const* end = p + s.size();
    while (p < end) {
        auto semi = (char const*)std::memchr(p, ';', end - p);
        char const* e = semi ? semi : end;
        Obj o;
        if (readObj(p, e, o)) {
            if (st) {
                st->objects++;
                st->maxX = std::max(st->maxX, o.x);
            }
            if (channel > 0 && (o.c1 == channel || o.c2 == channel)) {
                holds.push_back(o.x);
                if (st) st->holds++;
            } else if (std::find(ids.begin(), ids.end(), o.id) != ids.end()) {
                float r = std::fmod(o.rot, 360.f);
                if (r < 0.f) r += 360.f;
                bool up = !o.flipY && (r < 45.f || r > 315.f);
                if (!upright || up) {
                    spikes.push_back(o.x);
                    if (st) st->spikes++;
                }
            }
        }
        if (!semi) break;
        p = semi + 1;
    }
    std::vector<Evt> out;
    mergeInto(spikes, gap, false, out);
    mergeInto(holds, 31.f, true, out);
    std::sort(out.begin(), out.end(), [](Evt const& a, Evt const& b) { return a.x0 < b.x0; });
    return out;
}

// Read the level that is being played (pause menu)
inline std::string levelStringFromPlay(PlayLayer* pl) {
    std::string data;
    if (!pl || !pl->m_objects) return data;
    for (auto obj : CCArrayExt<GameObject*>(pl->m_objects)) {
        if (!obj) continue;
        data += std::string(obj->getSaveString(pl));
        data += ';';
    }
    return data;
}

// Read a level from its saved data (level page, before playing)
inline std::string levelStringFromLevel(GJGameLevel* lvl) {
    if (!lvl) return {};
    std::string raw = std::string(lvl->m_levelString);
    if (raw.empty()) return {};
    std::string dec = std::string(ZipUtils::decompressString(lvl->m_levelString, false, 0));
    return dec.empty() ? raw : dec;
}

inline std::string pack(std::vector<Evt> const& v) {
    std::string s;
    for (auto& e : v) s += fmt::format("{:.1f},{:.1f},{};", e.x0, e.x1, e.hold ? 1 : 0);
    return s;
}

inline std::vector<Evt> unpack(std::string const& s) {
    std::vector<Evt> v;
    char const* p = s.c_str();
    while (*p) {
        char* n = nullptr;
        Evt e;
        e.x0 = std::strtof(p, &n);
        if (*n != ',') break;
        p = n + 1;
        e.x1 = std::strtof(p, &n);
        if (*n != ',') break;
        p = n + 1;
        e.hold = std::strtol(p, &n, 10) != 0;
        if (*n != ';') break;
        p = n + 1;
        e.cursor = e.x0;
        v.push_back(std::move(e));
    }
    return v;
}

inline std::string keyFor(std::string const& name) { return "am-data:" + name; }

inline void store(std::string const& name, std::vector<Evt> const& v) {
    Mod::get()->setSavedValue<std::string>(keyFor(name), pack(v));
    (void)Mod::get()->saveData();
}

inline std::vector<Evt> load(std::string const& name) {
    return unpack(Mod::get()->getSavedValue<std::string>(keyFor(name), ""));
}

// ---------------- debug view helpers ----------------

struct CObj { float x = 0.f, y = 0.f; int id = 0, type = -1; GameObject* o = nullptr; };
inline std::vector<CObj> objCache; // every object of the level, sorted by x

template <class O> inline int objType(O* o) {
    if constexpr (requires { (int)o->m_objectType; }) return (int)o->m_objectType;
    else return -1;
}
template <class O> inline int objId(O* o) {
    if constexpr (requires { (int)o->m_objectID; }) return (int)o->m_objectID;
    else return -1;
}
template <class O> inline bool objRect(O* o, CCRect& out) {
    if constexpr (requires { out = o->getObjectRect(); }) { out = o->getObjectRect(); return true; }
    else return false;
}

template <class P> inline std::string modeName(P* p) {
    if (!p) return "-";
    if constexpr (requires { p->m_isShip; })  { if (p->m_isShip)  return "Ship"; }
    if constexpr (requires { p->m_isBird; })  { if (p->m_isBird)  return "UFO"; }
    if constexpr (requires { p->m_isBall; })  { if (p->m_isBall)  return "Ball"; }
    if constexpr (requires { p->m_isDart; })  { if (p->m_isDart)  return "Wave"; }
    if constexpr (requires { p->m_isRobot; }) { if (p->m_isRobot) return "Robot"; }
    if constexpr (requires { p->m_isSpider; }) { if (p->m_isSpider) return "Spider"; }
    if constexpr (requires { p->m_isSwing; }) { if (p->m_isSwing) return "Swing"; }
    return "Cube";
}
template <class P> inline bool gravityUp(P* p) {
    if constexpr (requires { p->m_isUpsideDown; }) return p && p->m_isUpsideDown;
    else return false;
}
template <class PL> inline int checkpointCount(PL* pl) {
    if constexpr (requires { pl->m_checkpointArray->count(); }) {
        return pl->m_checkpointArray ? (int)pl->m_checkpointArray->count() : 0;
    } else return -1;
}

inline void ensureCache(PlayLayer* pl) {
    if (!pl) return;
    if (state.cacheOwner == pl && !objCache.empty()) return;
    objCache.clear();
    state.cacheOwner = pl;
    if (!pl->m_objects) return;
    for (auto obj : CCArrayExt<GameObject*>(pl->m_objects)) {
        if (!obj) continue;
        CObj c;
        c.o = obj;
        c.x = obj->getPositionX();
        c.y = obj->getPositionY();
        c.id = objId(obj);
        c.type = objType(obj);
        if (c.id < 0) { // this game version hides the id: read it from the save string
            Obj r;
            std::string s = std::string(obj->getSaveString(pl));
            readObj(s.c_str(), s.c_str() + s.size(), r);
            c.id = r.id;
        }
        objCache.push_back(c);
    }
    std::sort(objCache.begin(), objCache.end(),
              [](CObj const& a, CObj const& b) { return a.x < b.x; });
}

// Finds the object near a point (level coordinates) and stores it for the inspector
inline bool pickAt(PlayLayer* pl, CCPoint p) {
    ensureCache(pl);
    CObj const* best = nullptr;
    float bd = 1e9f;
    auto lo = std::lower_bound(objCache.begin(), objCache.end(), p.x - 60.f,
                               [](CObj const& c, float v) { return c.x < v; });
    for (auto it = lo; it != objCache.end() && it->x <= p.x + 60.f; ++it) {
        float dx = it->x - p.x, dy = it->y - p.y, d = dx * dx + dy * dy;
        CCRect r;
        if (objRect(it->o, r) && r.containsPoint(p)) d *= 0.25f; // prefer what is under the finger
        if (d < bd) { bd = d; best = &*it; }
    }
    if (!best || bd > 60.f * 60.f) return false;

    Obj o;
    std::string s = std::string(best->o->getSaveString(pl));
    readObj(s.c_str(), s.c_str() + s.size(), o);
    auto& k = state.pick;
    k.has = true;
    k.id = best->id;
    k.type = best->type;
    k.x = best->x;
    k.y = best->y;
    k.c1 = o.c1;
    k.c2 = o.c2;
    k.rot = o.rot;
    k.flipY = o.flipY;
    return true;
}

inline std::string typeName(int t) {
    switch (t) {
        case 0: return "Solid";
        case 2: return "Hazard";
        case 7: return "Decoration";
        case 25: return "Slope";
        case 3: case 4: return "Gravity portal";
        case 5: case 6: case 16: case 19: case 26: case 27: case 33: case 41: return "Mode portal";
        case 17: case 18: return "Size portal";
        case 20: return "Trigger / modifier";
        case -1: return "Unknown";
        default: return fmt::format("Type {}", t);
    }
}

// Label for portals, shown by the "Portal labels" option
inline char const* portalLabel(int id) {
    switch (id) {
        case 10: return "Gravity: Down";
        case 11: return "Gravity: Up";
        case 12: return "Cube";
        case 13: return "Ship";
        case 47: return "Ball";
        case 111: return "UFO";
        case 660: return "Wave";
        case 745: return "Robot";
        case 1331: return "Spider";
        case 1933: return "Swing";
        case 99: return "Size: Normal";
        case 101: return "Size: Mini";
        case 45: return "Mirror: On";
        case 46: return "Mirror: Off";
        case 286: return "Dual: On";
        case 287: return "Dual: Off";
        case 200: return "Speed 0.5x";
        case 201: return "Speed 1x";
        case 202: return "Speed 2x";
        case 203: return "Speed 3x";
        case 1334: return "Speed 4x";
        case 747: return "Teleport";
        default: return nullptr;
    }
}

// Adds an object id to the "Spike Object IDs" setting
inline bool addDangerId(int id) {
    auto cur = parseIds(text("danger-ids"));
    if (std::find(cur.begin(), cur.end(), id) != cur.end()) return false;
    std::string s = text("danger-ids");
    if (!s.empty()) s += ",";
    s += std::to_string(id);
    Mod::get()->setSettingValue<std::string>("danger-ids", s);
    return true;
}

inline void resetLevelState() {
    state.checkpoints.clear();
    state.pick = Shared::Pick{};
    state.picking = false;
    state.down = false;
    state.cacheOwner = nullptr;
    objCache.clear();
}

} // namespace jg
