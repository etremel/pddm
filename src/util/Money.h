/*
 * Money.h
 *
 *  Created on: May 18, 2016
 *      Author: edward
 */

#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <cmath>

namespace pddm {
namespace util {

class Money {
    private:
        int amount;
        static const int scale = 1000000;
        static const char currency_symbol = '$'; //Eventually, this could be parameterized
    public:
        Money() : amount(0) {}
        Money(const double value) : amount(static_cast<int>(value * scale)) {}
        Money(Money&&) = default;
        //Default copy constructor and assignment operator will be auto-generated

        operator double() const {
            return double(amount) / scale;
        }
        Money& operator=(const double value) {
            amount = static_cast<int>(value * scale);
            return *this;
        }
        Money& operator+=(const Money& rhs) {
            amount += rhs.amount;
            return *this;
        }
        Money& operator-=(const Money& rhs) {
            amount -= rhs.amount;
            return *this;
        }
        Money& operator*=(const int rhs) {
            amount *= rhs;
            return *this;
        }
        Money& operator/=(const int rhs) {
            amount /= rhs;
            return *this;
        }
        Money& operator*=(const double rhs) {
            amount *= rhs;
            return *this;
        }
        Money& operator/=(const double rhs) {
            amount /= rhs;
            return *this;
        }

        friend Money operator+(Money lhs, const Money& rhs);
        friend Money operator-(Money lhs, const Money& rhs);
        friend Money operator*(Money lhs, const int rhs);
        friend Money operator*(Money lhs, const double rhs);
        friend Money operator*(const int lhs, const Money& rhs);
        friend Money operator*(const double lhs, const Money& rhs);

        friend bool operator==(const Money& lhs, const Money& rhs);
        friend bool operator!=(const Money& lhs, const Money& rhs);
        friend bool operator>(const Money& lhs, const Money& rhs);
        friend bool operator<(const Money& lhs, const Money& rhs);
        friend bool operator>=(const Money& lhs, const Money& rhs);
        friend bool operator<=(const Money& lhs, const Money& rhs);

        friend std::ostream& operator<<(std::ostream& os, const Money& obj);

};

//This makes a copy of lhs automatically, so we can use the mutative += on it
inline Money operator+(Money lhs, const Money& rhs) {
    lhs += rhs;
    return lhs;
}

inline Money operator-(Money lhs, const Money& rhs) {
    lhs -= rhs;
    return lhs;
}

inline Money operator*(Money lhs, const int rhs) {
    lhs *= rhs;
    return lhs;
}

inline Money operator*(Money lhs, const double rhs) {
    lhs *= rhs;
    return lhs;
}

inline Money operator/(Money lhs, const int rhs) {
    lhs /= rhs;
    return lhs;
}

inline Money operator/(Money lhs, const double rhs) {
    lhs /= rhs;
    return lhs;
}

inline Money operator*(const int lhs, const Money& rhs) { return rhs * lhs; }

inline Money operator*(const double lhs, const Money& rhs) { return rhs * lhs; }

inline bool operator==(const Money& lhs, const Money& rhs) {
    return lhs.amount == rhs.amount;
}

inline bool operator!=(const Money& lhs, const Money& rhs) {
    return !operator==(lhs, rhs);
}

inline bool operator<(const Money& lhs, const Money& rhs) {
    return lhs.amount < rhs.amount;
}

inline bool operator>(const Money& lhs, const Money& rhs) {
    return operator<(rhs, lhs);
}

inline bool operator<=(const Money& lhs, const Money& rhs) {
    return !operator>(lhs, rhs);
}

inline bool operator>=(const Money& lhs, const Money& rhs) {
    return !operator< (lhs,rhs);
}

inline std::ostream& operator<<(std::ostream& os, const Money& obj) {
    //Fix to exactly two decimal places
    int dollars = (int) floor(static_cast<double>(obj.amount) / Money::scale);
    int cents = (int) floor(static_cast<double>(obj.amount) / (Money::scale / 100)) - dollars * 100;
    std::stringstream sbuilder;
    sbuilder << Money::currency_symbol << dollars << "." << cents;
    os << sbuilder.str();
    return os;
}

} // namespace util

//Promote this to the top-level namespace as a shortcut
using Money = util::Money;

} //namespace psm


