// Verifies the knob PNG rotation math used by both editors. Reference: the
// vector fallback pointer is drawn along (cos(angle), sin(angle)) in screen
// coords with y DOWN, angle = 135 + 270*value. The shipped PNG face
// (assets/sprites/knob.png) has its printed indicator pointing UP at rest
// (verified by pixel analysis: bright marker pixels cluster at -90 deg = up).
#include <cmath>
#include <cstdio>
static const double tau = 6.28318530717958647692;

struct Vec { double x, y; };

// --- macOS: flipped view (isFlipped=YES), drawInRect respectFlipped:YES.
// AppKit maps the image through CTM' = F * M * F (F=diag(1,-1), M=[[c,-s],
// [s,c]] = CGContextRotateCTM(rot)). Conjugation gives [[c,s],[-s,c]];
// applied to the up indicator (0,-1) it maps to (-sin rot, cos rot).
// Editor code uses rot=(angle-90): (-sin(a-90), cos(a-90)) = (cos a, sin a)
// -- exactly the fallback pointer ray in y-down screen coords.
static Vec mac_indicator(double angleDeg) {
    double rot = (angleDeg - 90.0) * tau / 360.0;   // as coded in editor_mac.mm
    return {-std::sin(rot), std::cos(rot)};         // [[c,s],[-s,c]] @ up=(0,-1)
}
// --- Windows: GDI+ RotateTransform(theta) premultiplies the world transform
// by [[c,-s],[s,c]]; a device-space point p satisfies dev = M*world, so the
// content's own up indicator (0,-1) maps on screen to (-sin t, -cos t).
// Editor code uses theta=-(angle+90): (sin(a+90), cos(a+90)) = (cos a, sin a)
// -- exactly the fallback pointer ray in y-down screen coords.
static Vec win_indicator(double angleDeg) {
    double t = -(angleDeg + 90.0) * tau / 360.0;    // as coded in editor_win.cpp
    return {-std::sin(t), -std::cos(t)};            // [[c,-s],[s,c]] @ up=(0,-1)
}
static Vec expected(double angleDeg) {
    double a = angleDeg * tau / 360.0;
    return {std::cos(a), std::sin(a)};
}
int main() {
    int fails = 0;
    for (int i = 0; i <= 20; ++i) {
        double v = i / 20.0, angle = 135.0 + 270.0 * v;
        Vec e = expected(angle);
        auto chk = [&](const char* who, Vec g) {
            if (std::hypot(g.x - e.x, g.y - e.y) > 1e-9) {
                std::printf("FAIL %s v=%.2f got(%.3f,%.3f) want(%.3f,%.3f)\n", who, v, g.x, g.y, e.x, e.y); ++fails;
            }
        };
        chk("mac", mac_indicator(angle));
        chk("win", win_indicator(angle));
    }
    // sweep sanity: min down-left, mid up, max down-right (screen y-down)
    Vec a = expected(135), b = expected(270), c = expected(405);
    if (!(a.x < 0 && a.y > 0 && std::abs(b.x) < 1e-9 && b.y < 0 && c.x > 0 && c.y > 0)) {
        std::printf("FAIL sweep endpoints\n"); ++fails;
    }
    std::printf(fails ? "KNOB ROTATION: %d FAILURES\n" : "KNOB ROTATION: PASS (21 values, mac+win PNG match fallback pointer)\n", fails);
    return fails != 0;
}
