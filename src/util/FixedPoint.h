#pragma once

#include <iostream>
#include <functional>
#include <vector>
#include <cstddef>

namespace pddm {
namespace util {

/**
 * Implements a fixed-point decimal number.
 * @tparam Base The integer type that should be used to store the number
 * @tparam PrecisionBits The number of low-order bits of the integer type
 * that should be interpreted as places past the decimal point.
 */
template<typename Base, int PrecisionBits>
class FixedPoint {
    public:
        static const Base factor = 1 << (PrecisionBits - 1);
        using base_type = Base;

        FixedPoint() : m(0) { }
        FixedPoint(const double d) : m(static_cast<Base>(d * factor)) { }
        FixedPoint(const Base b) : m(b * factor) {}
        FixedPoint(const FixedPoint&) = default;
        FixedPoint(FixedPoint&&) = default;

        // friend functions
        friend constexpr FixedPoint operator+(const FixedPoint& x, const FixedPoint& y) { return FixedPoint{x.m + y.m}; }
        friend constexpr FixedPoint operator-(const FixedPoint& x, const FixedPoint& y) { return FixedPoint{x.m - y.m}; }
        friend constexpr FixedPoint operator*(const FixedPoint& x, const FixedPoint& y) { return FixedPoint{(x.m * y.m) >> PrecisionBits}; }
        friend constexpr FixedPoint operator/(const FixedPoint& x, const FixedPoint& y) { return FixedPoint{(x.m / y.m) * factor}; }


        FixedPoint& operator=(const FixedPoint& x) { m = x.m; return *this; }
        FixedPoint& operator+=(const FixedPoint& x) { return *this = (*this + x); }
        FixedPoint& operator-=(const FixedPoint& x) { return *this = (*this - x); }
        FixedPoint& operator*=(const FixedPoint& x) { return *this = (*this * x); }
        FixedPoint& operator/=(const FixedPoint& x) { return *this = (*this / x); }
        FixedPoint& operator*=(int x) { m *= x; return *this; }
        FixedPoint& operator/=(int x) { m /= x; return *this; }
        FixedPoint operator-() const { return FixedPoint(-m); }
        double toDouble() const { return double(m) / factor; }
        operator double() const { return toDouble(); }
        operator Base() const { return m; }

        // comparison operators
        friend constexpr bool operator==(const FixedPoint& x, const FixedPoint& y) { return x.m == y.m; }
        friend constexpr bool operator!=(const FixedPoint& x, const FixedPoint& y) { return x.m != y.m; }
        friend constexpr bool operator>(const FixedPoint& x, const FixedPoint& y) { return x.m > y.m; }
        friend constexpr bool operator<(const FixedPoint& x, const FixedPoint& y) { return x.m < y.m; }
        friend constexpr bool operator>=(const FixedPoint& x, const FixedPoint& y) { return x.m >= y.m; }
        friend constexpr bool operator<=(const FixedPoint& x, const FixedPoint& y) { return x.m <= y.m; }
    private:
        Base m;
};

template<typename Base, int PrecisionBits>
std::ostream& operator<<(std::ostream& out, const FixedPoint<Base, PrecisionBits>& value) {
    return out << value.toDouble();
}

} /* namespace util */
} /* namespace pddm */

namespace std {

template<typename Base, int PrecisionBits>
struct hash<pddm::util::FixedPoint<Base, PrecisionBits>> {
        size_t operator()(const pddm::util::FixedPoint<Base, PrecisionBits>& input) const {
            return std::hash<Base>()(static_cast<Base>(input));
        }
};

template<typename Base, int PrecisionBits, typename Allocator>
struct hash<vector<pddm::util::FixedPoint<Base, PrecisionBits>, Allocator>> {
        size_t operator()(const vector<pddm::util::FixedPoint<Base, PrecisionBits>, Allocator>& input) const {
            std::size_t seed = input.size();
            for (Base i : input) {
                seed ^= i + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            }
            return seed;
        }
};

} /* namespace std */
