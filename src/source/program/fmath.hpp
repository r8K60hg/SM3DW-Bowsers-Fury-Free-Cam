#pragma once

/*
 * Tiny self-contained float math so the module has no libm dependency.
 * Accuracy is far beyond what a camera needs (sin/cos ~1e-7, atan ~1e-5 rad).
 */

namespace fm {

    constexpr float Pi     = 3.14159265358979323846f;
    constexpr float TwoPi  = 6.28318530717958647692f;
    constexpr float HalfPi = 1.57079632679489661923f;
    constexpr float DegToRad = Pi / 180.0f;

    inline float Abs(float x) { return x < 0.0f ? -x : x; }
    inline float Min(float a, float b) { return a < b ? a : b; }
    inline float Max(float a, float b) { return a > b ? a : b; }
    inline float Clamp(float x, float lo, float hi) { return x < lo ? lo : (x > hi ? hi : x); }

    /* Single hardware instruction; avoids any errno / libm call. */
    inline float Sqrt(float x) {
        if (!(x > 0.0f))
            return 0.0f;
#if defined(__aarch64__)
        float r;
        __asm__("fsqrt %s0, %s1" : "=w"(r) : "w"(x));
        return r;
#else
        return __builtin_sqrtf(x); /* host-side unit tests only */
#endif
    }

    inline bool IsFinite(float x) {
        /* NaN fails every comparison; infinities fail the range check. */
        return x == x && x < 3.0e38f && x > -3.0e38f;
    }

    /* Wrap to [-Pi, Pi]. */
    inline float WrapPi(float x) {
        if (!IsFinite(x))
            return 0.0f;
        float k = x / TwoPi;
        /* Round to nearest integer without libm. */
        long n = static_cast<long>(k >= 0.0f ? k + 0.5f : k - 0.5f);
        return x - static_cast<float>(n) * TwoPi;
    }

    inline float Sin(float x) {
        x = WrapPi(x);
        /* Fold into [-Pi/2, Pi/2] using sin(Pi - x) = sin(x). */
        if (x > HalfPi)
            x = Pi - x;
        else if (x < -HalfPi)
            x = -Pi - x;
        const float x2 = x * x;
        /* Taylor series to x^11; max error ~6e-8 on [-Pi/2, Pi/2]. */
        return x * (1.0f + x2 * (-1.0f / 6.0f + x2 * (1.0f / 120.0f + x2 * (-1.0f / 5040.0f +
                    x2 * (1.0f / 362880.0f + x2 * (-1.0f / 39916800.0f))))));
    }

    inline float Cos(float x) { return Sin(x + HalfPi); }

    /* atan for |z| <= 1 (minimax polynomial, max error ~1e-5 rad). */
    inline float AtanUnit(float z) {
        const float z2 = z * z;
        return z * (0.9998660f + z2 * (-0.3302995f + z2 * (0.1801410f + z2 * (-0.0851330f + z2 * 0.0208351f))));
    }

    inline float Atan2(float y, float x) {
        const float ax = Abs(x), ay = Abs(y);
        if (ax == 0.0f && ay == 0.0f)
            return 0.0f;
        float a = (ax >= ay) ? AtanUnit(ay / ax) : HalfPi - AtanUnit(ax / ay);
        if (x < 0.0f)
            a = Pi - a;
        return y < 0.0f ? -a : a;
    }

    inline float Asin(float y) {
        y = Clamp(y, -1.0f, 1.0f);
        return Atan2(y, Sqrt(1.0f - y * y));
    }

    struct Vec3 {
        float x, y, z;

        Vec3 operator+(const Vec3& o) const { return { x + o.x, y + o.y, z + o.z }; }
        Vec3 operator-(const Vec3& o) const { return { x - o.x, y - o.y, z - o.z }; }
        Vec3 operator*(float s) const { return { x * s, y * s, z * s }; }
        Vec3& operator+=(const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }

        float Dot(const Vec3& o) const { return x * o.x + y * o.y + z * o.z; }
        Vec3 Cross(const Vec3& o) const { return { y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x }; }
        float Length() const { return Sqrt(Dot(*this)); }

        Vec3 Normalized(const Vec3& fallback) const {
            const float len = Length();
            if (len < 1.0e-6f)
                return fallback;
            return *this * (1.0f / len);
        }

        bool IsFinite() const { return fm::IsFinite(x) && fm::IsFinite(y) && fm::IsFinite(z); }
    };

    /* Frame-rate independent first-order smoothing factor (no exp() needed). */
    inline float SmoothAlpha(float dt, float timeConstant) {
        if (timeConstant <= 0.0f)
            return 1.0f;
        return dt / (timeConstant + dt);
    }

    inline float Lerp(float a, float b, float t) { return a + (b - a) * t; }
}
