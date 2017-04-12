/**
 * @file OStreams.h
 * Operator<< specializations for standard library types that are missing them.
 * @date Sep 6, 2016
 * @author edward
 */

#pragma once

#include <iterator>
#include <list>
#include <ostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace std {

//This is provided by mutils, apparently
//template <typename T>
//std::ostream& operator<< (std::ostream& out, const std::vector<T>& v) {
//  if ( !v.empty() ) {
//    out << '[';
//    std::copy (v.begin(), v.end(), std::ostream_iterator<T>(out, ", "));
//    out << "\b\b]";
//  }
//  return out;
//}

template <typename T>
std::ostream& operator<< (std::ostream& out, const std::list<T>& v) {
  if ( !v.empty() ) {
    out << '[';
    std::copy (v.begin(), v.end(), std::ostream_iterator<T>(out, ", "));
    out << "\b\b]";
  }
  return out;
}

template <typename T>
std::ostream& operator<< (std::ostream& out, const std::unordered_set<T>& s) {
  if ( !s.empty() ) {
    out << '[';
    std::copy (s.begin(), s.end(), std::ostream_iterator<T>(out, ", "));
    out << "\b\b]";
  }
  return out;
}

template <typename T>
std::ostream& operator<< (std::ostream& out, const std::unordered_multiset<T>& s) {
  if ( !s.empty() ) {
    out << '[';
    std::copy (s.begin(), s.end(), std::ostream_iterator<T>(out, ", "));
    out << "\b\b]";
  }
  return out;
}

}


