#pragma once

#include <iostream>
#include <iomanip>
#include <functional>
#include <vector>
#include <cstddef>
#include <mutils-serialization/SerializationSupport.hpp>

namespace pddm {
namespace util {

/**
 * Implements a fixed-point decimal number.
 * @tparam Base The integer type that should be used to store the number
 * @tparam PrecisionBits The number of low-order bits of the integer type
 * that should be interpreted as places past the decimal point.
 */
template<typename Base, int PrecisionBits>
class FixedPoint : public mutils::ByteRepresentable {
    public:
        static const constexpr Base factor = 1 << (PrecisionBits - 1);
        using base_type = Base;

        FixedPoint() : m(0) { }
        FixedPoint(const double d) : m(static_cast<Base>(d * factor)) { }
        FixedPoint(const FixedPoint&) = default;
        FixedPoint(FixedPoint&&) = default;

        // friend functions
        friend constexpr FixedPoint operator+(const FixedPoint& x, const FixedPoint& y) { return FixedPoint{x.m + y.m}; }
        friend constexpr FixedPoint operator-(const FixedPoint& x, const FixedPoint& y) { return FixedPoint{x.m - y.m}; }
        friend constexpr FixedPoint operator*(const FixedPoint& x, const FixedPoint& y) { return FixedPoint{(x.m * y.m) / factor}; }
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

        // comparison operators
        friend constexpr bool operator==(const FixedPoint& x, const FixedPoint& y) { return x.m == y.m; }
        friend constexpr bool operator!=(const FixedPoint& x, const FixedPoint& y) { return x.m != y.m; }
        friend constexpr bool operator>(const FixedPoint& x, const FixedPoint& y) { return x.m > y.m; }
        friend constexpr bool operator<(const FixedPoint& x, const FixedPoint& y) { return x.m < y.m; }
        friend constexpr bool operator>=(const FixedPoint& x, const FixedPoint& y) { return x.m >= y.m; }
        friend constexpr bool operator<=(const FixedPoint& x, const FixedPoint& y) { return x.m <= y.m; }

        //Serialization support
        std::size_t bytes_size() const { return mutils::bytes_size(m); }
        std::size_t to_bytes(char* buf) const { return mutils::to_bytes(m, buf); }
        void post_object(const std::function<void (char const * const,std::size_t)>& f) const {
            mutils::post_object(f, m);
        }
        static std::unique_ptr<FixedPoint> from_bytes(mutils::DeserializationManager<>* p, const char* buf) {
            Base internal_int;
            std::memcpy(&internal_int, buf, sizeof(Base));
            //std::make_unique bizarrely fails if used with a private constructor,
            //even though this context can access the private constructor
            return std::unique_ptr<FixedPoint>(new FixedPoint(internal_int));
        }

    private:
        Base m;
        explicit FixedPoint(const Base m) : m(m) {}
};

template<typename Base, int PrecisionBits>
std::ostream& operator<<(std::ostream& out, const FixedPoint<Base, PrecisionBits>& value) {
    //There's no good function for determining "number of decimal places you can express with x bits"
    //so this array just statically maps precision bits to decimal digits past the point
    static int sig_figs[] = {0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 6, 6};
    return out << std::fixed << std::setprecision(sig_figs[PrecisionBits]) << value.toDouble()
            << std::defaultfloat << std::setprecision(6); //reset stream to defaults before returning
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
