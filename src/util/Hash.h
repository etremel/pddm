/**
 * @file Hash.h
 * Tools for implementing std::hash for new types. Also plugs holes in the
 * standard library, such as the omission of std::hash specializations for
 * basic types like arrays.
 * @date Aug 10, 2016
 * @author edward
 */

#pragma once

#include <cstddef>
#include <type_traits>
#include <unordered_map>
#include <vector>


namespace pddm {
namespace util {

/**
 * Copy-and-paste of boost::hash_combine, magic numbers and all, so that I
 * don't have to use Boost just to have this function. Combines the provided
 * "seed" (existing hash value) with the std::hash of another object.
 * @param seed The starting value for the hash.
 * @param v The object whose hash value should be combined with seed.
 */
template <class T>
inline void hash_combine(std::size_t& seed, const T& v) {
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}


} /* namespace util */
} /* namespace pddm */


namespace std {
/**
 * Sensible implementation of std::hash for vectors of numeric types, using the
 * same code as boost::hash_combine.
 */
template<typename T, typename Allocator>
struct hash<vector<T, Allocator>> {
        static_assert(is_arithmetic<T>::value, "Generic std::vector hash only works for numeric types!");
    size_t operator()(const vector<T, Allocator> & vec) const {
        size_t seed = vec.size();
        for (const auto& i : vec) {
            seed ^= i + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        return seed;
    }

};


/*
 * Trivial implementations of std::hash for plain arrays of primitive types,
 * using boost::hash_combine. All primitive types have a std::hash specialization
 * already, but for some reason their array versions don't.
 */

template<size_t N>
struct hash<char[N]> {
        size_t operator()(const char (& input) [N]) const {
            size_t seed = 1;
            for(const auto& i : input) {
                pddm::util::hash_combine(seed, i);
            }
            return seed;
        }
};

template<size_t N>
struct hash<unsigned char[N]> {
        size_t operator()(const unsigned char (& input) [N]) const {
            size_t seed = 1;
            for(const auto& i : input) {
                pddm::util::hash_combine(seed, i);
            }
            return seed;
        }
};

template<size_t N>
struct hash<int[N]> {
        size_t operator()(const int (& input) [N]) const {
            size_t seed = 1;
            for(const auto& i : input) {
                pddm::util::hash_combine(seed, i);
            }
            return seed;
        }
};

template<size_t N>
struct hash<unsigned int[N]> {
        size_t operator()(const unsigned int (& input) [N]) const {
            size_t seed = 1;
            for(const auto& i : input) {
                pddm::util::hash_combine(seed, i);
            }
            return seed;
        }
};


/**
 * Trivial implementation of std::hash for std::array, using hash_combine. Only
 * works if the array's element type already has a std::hash defined for it.
 */
template<typename E, size_t N>
struct hash<array<E, N>> {
        size_t operator()(const array<E, N>& input) const {
            size_t seed = 1;
            for(const auto& i : input) {
                pddm::util::hash_combine(seed, i);
            }
            return seed;
        }
};

}
