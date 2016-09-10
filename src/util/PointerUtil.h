/*
 * PointerUtil.h
 *
 *  Created on: May 25, 2016
 *      Author: edward
 */

#pragma once

#include <stddef.h>
#include <memory>
#include <ostream>
#include <unordered_map>
#include <unordered_set>

namespace pddm {

namespace util {

/**
 * Re-implementation of std::greater<T> that compares the actual objects
 * referenced by two pointers, rather than the pointers themselves.
 */
template <typename T>
struct ptr_greater {
        bool operator()(const std::shared_ptr<T>& lhs, const std::shared_ptr<T>& rhs) const {
            return (lhs == nullptr || rhs == nullptr) ? lhs > rhs : *lhs > *rhs;
        }
};

/**
 * Re-implementation of std::equal_to<T> that compares the actual objects
 * referenced by two pointers, rather than the pointers themselves.
 */
template<typename T>
struct ptr_equal {
        bool operator()(const std::shared_ptr<T>& lhs, const std::shared_ptr<T>& rhs) const {
            return (lhs == nullptr || rhs == nullptr) ? lhs == rhs : *lhs == *rhs;
        }
};

/**
 * Generic "compare type" for pointers that takes a Compare functor type and
 * applies it to the objects referenced by the pointers.
 */
template<typename T, typename Compare>
struct ptr_comparator {
        bool operator()(const std::shared_ptr<T>& lhs, const std::shared_ptr<T>& rhs) const {
            return Compare()(*lhs, *rhs);
        }
};

/**
 * Hash functor for shared_ptr that hashes the object referenced by the pointer,
 * rather than the pointer itself.
 */
template<typename T>
struct ptr_hash {
        size_t operator()(const std::shared_ptr<T>& input) const {
            return input == nullptr ? std::hash<std::shared_ptr<T>>()(input) : std::hash<T>()(*input);
        }
};

/** Specialization of std::unordered_set that stores objects "by pointer" but
 * uses their actual values to compare. */
template<typename T>
using unordered_ptr_set = std::unordered_set<std::shared_ptr<T>, ptr_hash<T>, ptr_equal<T>>;

/** Specialization of std::unordered_multiset that stores objects "by pointer" but
 * uses their actual values to compare. */
template<typename T>
using unordered_ptr_multiset = std::unordered_multiset<std::shared_ptr<T>, ptr_hash<T>, ptr_equal<T>>;

/**
 * Helper for writing stream-ouput operators that prints a shared_ptr "by value":
 * either prints the value pointed to by the pointer (using its operator<<), or
 * prints the string "nullptr".
 * @param out The stream to print to
 * @param ptr The pointer to print in this stream
 */
template<typename T>
inline void print_ptr(std::ostream& out, const std::shared_ptr<T>& ptr) {
    if(ptr == nullptr)
        out << "nullptr";
    else
        out << *ptr;
}

/**
 * Stream-output operator overload specialized for pointer-sets; attempts to
 * dereference each element before calling operator<< on it, or prints "nullptr"
 * if the element is a null pointer.
 * @param out The stream to print to
 * @param s The unordered_ptr_set to print
 * @return The same stream as {@code out}
 */
template <typename T>
std::ostream& operator<< (std::ostream& out, const unordered_ptr_set<T>& s) {
  if ( !s.empty() ) {
    out << '[';
    for(const auto& ptr : s) {
        print_ptr(out, ptr);
        out << ", ";
    }
    out << "\b\b]";
  }
  return out;
}

template <typename T>
std::ostream& operator<< (std::ostream& out, const unordered_ptr_multiset<T>& s) {
  if ( !s.empty() ) {
    out << '[';
    for(const auto& ptr : s) {
        print_ptr(out, ptr);
        out << ", ";
    }
    out << "\b\b]";
  }
  return out;
}

}

}


