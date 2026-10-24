#include "TimingBar.hpp"
using namespace jg;

static ccColor4F const GREEN = rgba(0.20f, 1.00f, 0.45f);
static ccColor4F const RED   = rgba(1.00f, 0.25f, 0.32f);
static ccColor4F const AMBER = rgba(1.00f, 0.78f, 0.20f);
static ccColor4F const WHITE = rgba(1.00f, 1.00f, 1.00f);

void TimingBar::drawStatic() {
    m_bg->clear();
    // soft outer neon halo
    for (int i = 3; i >= 1; --i) {
        roundRect(m_bg, -i * 2.f, -i * 2.f, m_w + i * 2.f, m_h + i * 2.f, 12.f + i * 2.f,
                  rgba(0, 0, 0, 0), 2.f, col(m_accent, 0.06f * (4 - i)));
    }
    // panel
    roundRect(m_bg, 0, 0, m_w, m_h, 12.f, col(rgba(0.03f, 0.04f, 0.07f), 0.82f),
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
        if (e.hit)       { fill = rgba(0.06f, 0.50f, 0.22f); rim = GREEN; glow = GREEN; }
        else if (e.done) { fill = rgba(0.42f, 0.09f, 0.12f); rim = RED; }
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
        roundRect(m_dyn, x0 + 3, sa, x1 - 3, sb, 3.f, col(s.ok ? GREEN : RED, 0.9f));
        if (m_glowOn && s.ok) glowRect(x0 + 3, sa, x1 - 3, sb, GREEN);
    }
}

void TimingBar::drawLine(float dt) {
    ccColor4F lc = m_down ? GREEN : m_accent;
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

    if (m_flash > 0.f) { // ripple when a press lands
        float k = 1.f - m_flash;
        m_glow->drawSegment({m_pad, m_yLine}, {m_w - m_pad, m_yLine}, 4.f + k * 22.f,
                            col(GREEN, 0.35f * m_flash));
        m_flash = std::max(0.f, m_flash - dt * 3.5f);
    }
}

void TimingBar::drawFrame(float lineX, float dt) {
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
