#pragma once

namespace pddm {
namespace util {

#include <iostream>

template<typename Base, int PrecisionBits>
class FixedPoint {
    public:
        static const Base factor = 1 << (PrecisionBits - 1);

        FixedPoint() : m(0) { }
        FixedPoint(const double d) : m(static_cast<Base>(d * factor)) { }
        FixedPoint(FixedPoint&&) = default;

        FixedPoint& operator+=(const FixedPoint& x) { m += x.m; return *this; }
        FixedPoint& operator-=(const FixedPoint& x) { m -= x.m; return *this; }
        FixedPoint& operator*=(const FixedPoint& x) { m *= x.m; m >>= PrecisionBits; return *this; }
        FixedPoint& operator/=(const FixedPoint& x) { m /= x.m; m *= factor; return *this; }
        FixedPoint& operator*=(int x) { m *= x; return *this; }
        FixedPoint& operator/=(int x) { m /= x; return *this; }
        FixedPoint operator-() { return FixedPoint(-m); }
        double toDouble() const { return double(m) / factor; }
        operator double() const { return toDouble(); }

        // friend functions
        friend FixedPoint operator+(FixedPoint x, const FixedPoint& y) { return x += y; }
        friend FixedPoint operator-(FixedPoint x, const FixedPoint& y) { return x -= y; }
        friend FixedPoint operator*(FixedPoint x, const FixedPoint& y) { return x *= y; }
        friend FixedPoint operator/(FixedPoint x, const FixedPoint& y) { return x /= y; }

        // comparison operators
        friend bool operator==(const FixedPoint& x, const FixedPoint& y) { return x.m == y.m; }
        friend bool operator!=(const FixedPoint& x, const FixedPoint& y) { return x.m != y.m; }
        friend bool operator>(const FixedPoint& x, const FixedPoint& y) { return x.m > y.m; }
        friend bool operator<(const FixedPoint& x, const FixedPoint& y) { return x.m < y.m; }
        friend bool operator>=(const FixedPoint& x, const FixedPoint& y) { return x.m >= y.m; }
        friend bool operator<=(const FixedPoint& x, const FixedPoint& y) { return x.m <= y.m; }
    private:
        Base m;
};

} /* namespace util */
} /* namespace psm */
