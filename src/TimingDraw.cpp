#include "TimingBar.hpp"
using namespace jg;

static ccColor4F const AMBER = rgba(1.00f, 0.78f, 0.20f);
static ccColor4F const WHITE = rgba(1.00f, 1.00f, 1.00f);

void TimingBar::drawStatic() {
    if (m_ladder) { this->drawStaticLadder(); return; }
    m_bg->clear();
    // soft outer neon halo
    for (int i = 3; i >= 1; --i) {
        roundRect(m_bg, -i * 2.f, -i * 2.f, m_w + i * 2.f, m_h + i * 2.f, 12.f + i * 2.f,
                  rgba(0, 0, 0, 0), 2.f, col(m_accent, 0.06f * (4 - i)));
    }
    // panel
    roundRect(m_bg, 0, 0, m_w, m_h, 12.f, col(rgba(0.03f, 0.04f, 0.07f), m_hc ? 0.97f : 0.82f),
              1.6f, col(m_accent, 0.75f));
    // lane
    roundRect(m_bg, m_pad, m_yBot, m_w - m_pad, m_yTop, 8.f,
              col(rgba(0.01f, 0.02f, 0.04f), 0.70f), 1.f, col(WHITE, 0.07f));
    // exit zone under the line (where the green/red trail shows)
    roundRect(m_bg, m_pad + 1, m_yBot + 1, m_w - m_pad - 1, m_yLine - 2, 6.f, col(m_accent, 0.06f));
    // header divider
    m_bg->drawSegment({m_pad + 6, m_yTop + 6}, {m_w - m_pad - 6, m_yTop + 6}, 0.6f,
                      col(m_accent, 0.4f));
}

void TimingBar::glowRect(float x0, float y0, float x1, float y1, ccColor4F c) {
    for (int i = 0; i < 3; ++i) {
        float g = 2.f + i * 3.f;
        roundRect(m_glow, x0 - g, y0 - g, x1 + g, y1 + g, 6.f + g, col(c, 0.10f - i * 0.03f));
    }
}

void TimingBar::drawEvent(Evt const& e, float ya, float yb) {
    float x0 = m_pad + 5.f, x1 = m_w - m_pad - 5.f;
    float lo = std::max(ya, m_yBot + 1.f), hi = std::min(yb, m_yTop - 1.f);
    if (hi - lo < 5.f) hi = lo + 5.f;
    bool atLine = ya <= m_yLine + 4.f && yb >= m_yLine - 4.f;
    float pulse = 0.65f + 0.35f * std::sin(m_time * 14.f);

    ccColor4F fill = rgba(0.17f, 0.18f, 0.22f); // dark grey = waiting
    ccColor4F rim = rgba(0.42f, 0.44f, 0.54f);
    ccColor4F glow = rgba(0, 0, 0, 0);

    if (!e.hold) {
        if (e.hit)       { fill = rgba(0.06f, 0.50f, 0.22f); rim = m_good; glow = m_good; }
        else if (e.done) { fill = rgba(0.42f, 0.09f, 0.12f); rim = m_bad; }
        else if (atLine) { fill = rgba(0.50f * pulse, 0.36f * pulse, 0.06f); rim = AMBER; glow = AMBER; }
    } else if (atLine) {
        rim = m_accent;
    }

    roundRect(m_dyn, x0, lo, x1, hi, 5.f, col(fill, 0.96f), 1.4f, col(rim));
    if (m_glowOn && glow.a > 0.f) glowRect(x0, lo, x1, hi, glow);
    if (!e.hold) return;

    // hold segments: down chevrons = "keep holding"
    float cx = m_w * 0.5f;
    for (float y = lo + 9.f; y < hi - 6.f; y += 15.f) {
        tri(m_dyn, {cx - 6, y + 3}, {cx + 6, y + 3}, {cx, y - 4}, col(WHITE, 0.16f));
    }
    // already-passed part: green while held, red when released
    for (auto& s : e.spans) {
        float sa = std::max(yOf(s.x0), m_yBot + 1.f);
        float sb = std::min(yOf(s.x1), m_yTop - 1.f);
        if (sb - sa < 1.f) continue;
        roundRect(m_dyn, x0 + 3, sa, x1 - 3, sb, 3.f, col(s.ok ? m_good : m_bad, 0.9f));
        if (m_glowOn && s.ok) glowRect(x0 + 3, sa, x1 - 3, sb, m_good);
    }
}

void TimingBar::drawLine(float dt) {
    ccColor4F lc = m_down ? m_good : m_accent;
    float pulse = 0.6f + 0.4f * std::sin(m_time * 6.f);
    float zone = std::min(m_preSec * m_speed * m_pxU, 46.f);

    roundRect(m_dyn, m_pad + 1, m_yLine, m_w - m_pad - 1, m_yLine + zone, 2.f, col(lc, 0.10f));
    m_dyn->drawSegment({m_pad, m_yLine}, {m_w - m_pad, m_yLine}, 1.5f, col(lc));
    if (m_glowOn) {
        m_glow->drawSegment({m_pad, m_yLine}, {m_w - m_pad, m_yLine}, 5.f, col(lc, 0.20f * pulse));
        m_glow->drawSegment({m_pad, m_yLine}, {m_w - m_pad, m_yLine}, 10.f, col(lc, 0.07f * pulse));
    }
    tri(m_dyn, {1, m_yLine + 7}, {1, m_yLine - 7}, {10, m_yLine}, col(lc));
    tri(m_dyn, {m_w - 1, m_yLine + 7}, {m_w - 1, m_yLine - 7}, {m_w - 10, m_yLine}, col(lc));

    if (m_reduce) m_flash = 0.f;
    if (m_flash > 0.f) { // ripple when a press lands
        float k = 1.f - m_flash;
        m_glow->drawSegment({m_pad, m_yLine}, {m_w - m_pad, m_yLine}, 4.f + k * 22.f,
                            col(m_good, 0.35f * m_flash));
        m_flash = std::max(0.f, m_flash - dt * 3.5f);
    }
}

void TimingBar::drawFrame(float lineX, float dt) {
    if (m_ladder) { this->drawFrameLadder(lineX, dt); return; }
    m_lineX = lineX;
    m_time += dt;
    m_dyn->clear();
    m_glow->clear();

    float xBottom = lineX - (m_yLine - m_yBot) / m_pxU; // world x at the lane bottom
    if (m_grid) {
        float step = 120.f; // every 4 blocks, scrolls with your speed
        float x = std::floor(xBottom / step) * step;
        for (int n = 0; n < 64; ++n, x += step) {
            float y = yOf(x);
            if (y > m_yTop) break;
            if (y < m_yBot) continue;
            m_dyn->drawSegment({m_pad + 3, y}, {m_w - m_pad - 3, y}, 0.5f, col(WHITE, 0.08f));
        }
    }

    while (m_first < m_events.size() && m_events[m_first].x1 < xBottom - 10.f) ++m_first;
    for (size_t i = m_first; i < m_events.size(); ++i) {
        auto& e = m_events[i];
        float ya = yOf(e.x0), yb = yOf(e.x1);
        if (ya > m_yTop) break;
        if (yb < m_yBot) continue;
        this->drawEvent(e, ya, yb);
    }

    this->drawLine(dt);

    // fade at the top so segments emerge out of the dark
    float fh = (m_yTop - m_yLine) * 0.22f;
    for (int i = 0; i < 8; ++i) {
        float t = i / 8.f;
        roundRect(m_dyn, m_pad + 1, m_yTop - fh * (t + 0.125f), m_w - m_pad - 1, m_yTop - fh * t,
                  0.5f, col(rgba(0.02f, 0.03f, 0.05f), 0.85f * (1.f - t)));
    }
}

// ================= ladder style =================
// A perspective track: two rails that narrow toward the top, white rungs that
// get bigger as they come closer, and a hit line with < > brackets at the end.

static constexpr float kPersp = 0.6f;

float TimingBar::depthScale(float t) const {
    return t <= 0.f ? 1.f : 1.f / (1.f + kPersp * t);
}

float TimingBar::depthY(float t) const {
    float s1 = 1.f / (1.f + kPersp);
    float H = m_yTop - m_yLine;
    if (t <= 0.f) return m_yLine + t * H * kPersp / (1.f - s1); // below the line: straight on
    t = std::min(t, 1.f);
    return m_yLine + H * (1.f - 1.f / (1.f + kPersp * t)) / (1.f - s1);
}

// Trapezoid between two depths (t = 0 at the line, 1 at the top)
void TimingBar::quadT(CCDrawNode* d, float ta, float tb, float minH, float inset, ccColor4F fill,
                      float bw, ccColor4F border) {
    float cx = m_w * 0.5f, hw = (m_w * 0.5f - m_pad - 2.f) * inset;
    float ya = depthY(ta), yb = depthY(tb);
    float sa = depthScale(ta), sb = depthScale(tb);
    if (yb - ya < minH * sa) yb = ya + minH * sa;
    CCPoint v[4] = {CCPoint(cx - hw * sa, ya), CCPoint(cx + hw * sa, ya),
                    CCPoint(cx + hw * sb, yb), CCPoint(cx - hw * sb, yb)};
    d->drawPolygon(v, 4, fill, bw, border);
}

void TimingBar::drawStaticLadder() {
    m_bg->clear();
    float cx = m_w * 0.5f, hw = m_w * 0.5f - m_pad - 2.f;
    float s1 = depthScale(1.f);
    ccColor4F rail = col(WHITE, 0.6f);
    m_bg->drawSegment(CCPoint(cx - hw, m_yLine - 4.f), CCPoint(cx - hw * s1, m_yTop), 1.3f, rail);
    m_bg->drawSegment(CCPoint(cx + hw, m_yLine - 4.f), CCPoint(cx + hw * s1, m_yTop), 1.3f, rail);
}

void TimingBar::drawRung(jg::Evt const& e, float t0, float t1, float tmin) {
    float a = std::max(t0, tmin), b = std::min(t1, 1.f);
    if (b <= a) b = a + 0.01f;
    bool atLine = t0 <= 0.03f && t1 >= -0.03f;
    float pulse = 0.7f + 0.3f * std::sin(m_time * 14.f);

    ccColor4F fill = rgba(0.94f, 0.94f, 0.96f, 0.96f);
    ccColor4F glow = rgba(0.f, 0.f, 0.f, 0.f);
    if (!e.hold) {
        if (e.hit)       { fill = m_good; glow = m_good; }
        else if (e.done) { fill = m_bad; }
        else if (atLine) { fill = rgba(1.f, 0.84f * pulse, 0.30f * pulse, 1.f); glow = AMBER; }
    } else if (atLine) {
        glow = m_accent;
    }

    quadT(m_dyn, a, b, 7.f, 0.88f, col(fill), 0.8f, col(WHITE, 0.9f));
    if (m_glowOn && glow.a > 0.f) quadT(m_glow, a, b, 7.f, 0.98f, col(glow, 0.22f), 0.f, ccColor4F{0, 0, 0, 0});

    if (!e.hold) return;
    // hold: a darker stripe down the middle, and the part you held / let go of
    quadT(m_dyn, a, b, 7.f, 0.10f, col(rgba(0.30f, 0.31f, 0.36f), 0.8f), 0.f, ccColor4F{0, 0, 0, 0});
    for (auto& s : e.spans) {
        float sa = std::max((s.x0 - m_lineX) / m_viewUnits, tmin);
        float sb = std::min((s.x1 - m_lineX) / m_viewUnits, 1.f);
        if (sb <= sa) continue;
        quadT(m_dyn, sa, sb, 3.f, 0.80f, col(s.ok ? m_good : m_bad, 0.95f), 0.f, ccColor4F{0, 0, 0, 0});
        if (m_glowOn && s.ok) quadT(m_glow, sa, sb, 3.f, 0.90f, col(m_good, 0.2f), 0.f, ccColor4F{0, 0, 0, 0});
    }
}

void TimingBar::drawLineLadder() {
    float cx = m_w * 0.5f, hw = m_w * 0.5f - m_pad - 2.f, y = m_yLine;
    ccColor4F lc = m_down ? m_good : rgba(1.f, 1.f, 1.f, 1.f);
    m_dyn->drawSegment(CCPoint(cx - hw - 3.f, y), CCPoint(cx + hw + 3.f, y), 2.2f, col(lc));
    float xl = cx - hw - 9.f, xr = cx + hw + 9.f; // the < > brackets at both ends
    m_dyn->drawSegment(CCPoint(xl + 10.f, y + 14.f), CCPoint(xl, y), 2.f, col(lc));
    m_dyn->drawSegment(CCPoint(xl, y), CCPoint(xl + 10.f, y - 14.f), 2.f, col(lc));
    m_dyn->drawSegment(CCPoint(xr - 10.f, y + 14.f), CCPoint(xr, y), 2.f, col(lc));
    m_dyn->drawSegment(CCPoint(xr, y), CCPoint(xr - 10.f, y - 14.f), 2.f, col(lc));
    if (m_glowOn) {
        float pulse = 0.6f + 0.4f * std::sin(m_time * 6.f);
        m_glow->drawSegment(CCPoint(cx - hw, y), CCPoint(cx + hw, y), 5.f, col(lc, 0.18f * pulse));
    }
    if (m_reduce) m_flash = 0.f;
    if (m_flash > 0.f) { // ripple when a press lands
        m_glow->drawSegment(CCPoint(cx - hw, y), CCPoint(cx + hw, y), 4.f + (1.f - m_flash) * 20.f,
                            col(m_good, 0.35f * m_flash));
    }
}

void TimingBar::drawFrameLadder(float lineX, float dt) {
    m_lineX = lineX;
    m_time += dt;
    m_dyn->clear();
    m_glow->clear();

    // how far below the line the passed part stays visible
    float H = m_yTop - m_yLine, s1 = 1.f / (1.f + kPersp);
    float tmin = -(m_yLine - m_yBot) * (1.f - s1) / (H * kPersp);
    float xBottom = lineX + tmin * m_viewUnits;

    while (m_first < m_events.size() && m_events[m_first].x1 < xBottom - 10.f) ++m_first;
    for (size_t i = m_first; i < m_events.size(); ++i) {
        auto& e = m_events[i];
        float t0 = (e.x0 - lineX) / m_viewUnits, t1 = (e.x1 - lineX) / m_viewUnits;
        if (t0 > 1.f) break;
        if (t1 < tmin) continue;
        this->drawRung(e, t0, t1, tmin);
    }

    this->drawLineLadder();
    if (m_flash > 0.f) m_flash = std::max(0.f, m_flash - dt * 3.5f);
}
