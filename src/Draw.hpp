#pragma once
#include "Data.hpp"

namespace jg {

inline ccColor4F rgba(float r, float g, float b, float a = 1.f) {
    return ccColor4F{r, g, b, a};
}

inline ccColor3B c3(float r, float g, float b) {
    return ccColor3B{(GLubyte)(r * 255.f), (GLubyte)(g * 255.f), (GLubyte)(b * 255.f)};
}

inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }

inline ccColor4F mixc(ccColor4F a, ccColor4F b, float t) {
    return ccColor4F{lerpf(a.r, b.r, t), lerpf(a.g, b.g, t), lerpf(a.b, b.b, t), lerpf(a.a, b.a, t)};
}

inline float easeOutBack(float x) {
    float c1 = 1.70158f, c3v = c1 + 1.f, p = x - 1.f;
    return 1.f + c3v * p * p * p + c1 * p * p;
}

inline ccColor4F accentFor(std::string const& name) {
    if (name == "Magenta") return rgba(1.00f, 0.30f, 0.85f);
    if (name == "Lime")    return rgba(0.55f, 1.00f, 0.25f);
    if (name == "Amber")   return rgba(1.00f, 0.72f, 0.15f);
    if (name == "Violet")  return rgba(0.65f, 0.45f, 1.00f);
    if (name == "Silver")  return rgba(0.84f, 0.86f, 0.90f);
    return rgba(0.15f, 0.90f, 1.00f); // Cyan
}

// "hit" and "miss" colors, swapped for colorblind-friendly pairs
inline ccColor4F goodColor() {
    auto m = text("colorblind");
    if (m == "Off") return rgba(0.20f, 1.00f, 0.45f);
    if (m == "Tritan") return rgba(0.10f, 0.85f, 0.80f);
    return rgba(0.25f, 0.65f, 1.00f);
}
inline ccColor4F badColor() {
    auto m = text("colorblind");
    if (m == "Off") return rgba(1.00f, 0.25f, 0.32f);
    if (m == "Tritan") return rgba(1.00f, 0.30f, 0.35f);
    return rgba(1.00f, 0.62f, 0.10f);
}

// Rounded rectangle from (x0,y0) to (x1,y1), optional border
inline void roundRect(CCDrawNode* d, float x0, float y0, float x1, float y1, float r,
                      ccColor4F fill, float bw = 0.f,
                      ccColor4F border = ccColor4F{0, 0, 0, 0}) {
    if (x1 <= x0 || y1 <= y0) return;
    r = std::max(0.01f, std::min({r, (x1 - x0) * 0.5f, (y1 - y0) * 0.5f}));
    constexpr int N = 4;
    float cx[4] = {x1 - r, x0 + r, x0 + r, x1 - r};
    float cy[4] = {y1 - r, y1 - r, y0 + r, y0 + r};
    std::vector<CCPoint> pts;
    pts.reserve(4 * (N + 1));
    for (int c = 0; c < 4; ++c) {
        for (int i = 0; i <= N; ++i) {
            float a = (c * 90.f + 90.f * i / N) * 0.0174533f;
            pts.push_back(CCPoint(cx[c] + r * std::cos(a), cy[c] + r * std::sin(a)));
        }
    }
    d->drawPolygon(pts.data(), (unsigned int)pts.size(), fill, bw, border);
}

inline void tri(CCDrawNode* d, CCPoint a, CCPoint b, CCPoint c, ccColor4F col) {
    CCPoint v[3] = {a, b, c};
    d->drawPolygon(v, 3, col, 0.f, ccColor4F{0, 0, 0, 0});
}

} // namespace jg
