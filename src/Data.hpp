#pragma once
#include <Geode/Geode.hpp>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

using namespace geode::prelude;

namespace jg {

struct Span { float x0, x1; bool ok; };

// One segment on the bar (spike cluster = tap, marker run = hold)
struct Evt {
    float x0 = 0.f, x1 = 0.f;
    bool hold = false;
    float cursor = 0.f;
    bool hit = false, done = false;
    std::vector<Span> spans;
};

struct Stats { int objects = 0, spikes = 0, holds = 0; float maxX = 0.f; };

// Shared runtime state (menu, HUD, bar)
struct Shared {
    std::string name;
    Stats stats;
    size_t segs = 0;
    bool has = false;
    int attempts = 0, best = 0;
};
inline Shared state;

inline double fnum(char const* k) { return Mod::get()->getSettingValue<double>(k); }
inline int inum(char const* k) { return (int)Mod::get()->getSettingValue<int64_t>(k); }
inline bool flag(char const* k) { return Mod::get()->getSettingValue<bool>(k); }
inline std::string text(char const* k) { return Mod::get()->getSettingValue<std::string>(k); }

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

// Read the level that is currently being played (used from the pause menu)
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

// Read a level from its saved data (used from the level page, before playing)
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

inline std::string keyFor(std::string const& name) { return "jg-data:" + name; }

inline void store(std::string const& name, std::vector<Evt> const& v) {
    Mod::get()->setSavedValue<std::string>(keyFor(name), pack(v));
    (void)Mod::get()->saveData();
}

inline std::vector<Evt> load(std::string const& name) {
    return unpack(Mod::get()->getSavedValue<std::string>(keyFor(name), ""));
}

} // namespace jg
