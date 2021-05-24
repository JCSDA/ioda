#pragma once
/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! @file Options.h
* @brief Quick and easy key-value container that stringifies all values.
*/
#include <exception>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>

#include "../defs.h"

namespace ioda {
/*! @brief Quick and easy key-value container that stringifies all values.
* @details Used in the ioda error system.
*/
class Options {
  std::map<std::string, std::string> mapStr_;

public:
  /// List all stored values.
  inline void enumVals(std::ostream& out, int level = 1) const {
    for (const auto& v : mapStr_) {
      out.write("\t", level);
      out << v.first << ":\t" << v.second << std::endl;
    }
  }
  /// Does a key of the specified name exist?
  inline bool has(const std::string& key) const noexcept {
    if (mapStr_.count(key)) return true;
    return false;
  }
  /// Retrieves an option.
  template <class T>
  T get(const std::string& key, bool& result) const {
    if (!has(key)) {
      result = false;
      return T();
    }
    std::string valS = mapStr_.at(key);
    std::istringstream i{valS};
    T res{};
    i >> res;
    result = true;
    return res;
  }
  /// Retrieves an option. Returns defaultval if nonexistant.
  template <class T>
  T get(const std::string& key, const T& defaultval, bool& result) const {
    if (!has(key)) {
      result = false;
      return defaultval;
    }
    return get<T>(key, result);
  }
  /// Adds or replaces an option.
  template <class T>
  Options& set(const std::string& key, const T& value) {
    std::ostringstream o;
    o << value;
    std::string valS = o.str();
    mapStr_[key]     = valS;
    return *this;
  }
  /// Adds an option. Throws if the same name already exists.
  template <class T>
  Options& add(const std::string& key, const T& value) {
    if (has(key)) throw std::logic_error("Key already exists.");
    return this->set<T>(key, value);
  }
};

}  // namespace ioda
