/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef CONTAINERS_IVIEW_H_
#define CONTAINERS_IVIEW_H_

#include <cstdint>
#include <string>
#include <vector>

namespace osdf {

/// \class IView
/// \brief The pure-abstract base class for the read-only data view container. This class
/// represents an interface and sets the contract to which which derived data view containers must
/// adhere. There are are currently no member variables, and no function definitions. However, this
/// class does have an explicit constructor as a place to initialise future potential member
/// variables. This could be useful if future extenstions to derived classes leads to common
/// functionality being moved here. At present this class allows derived types to be handled using
/// a pointer to this class, and for the interface to this class to be available.

class IView {
 public:
    /// \brief The default, currently empty constructor.
  IView() {}

  IView(IView&&)                 = delete;
  IView(const IView&)            = delete;
  IView& operator=(IView&&)      = delete;
  IView& operator=(const IView&) = delete;

  /// \brief The following functions are used to copy data from a column. The derived classes use
  /// templated functions to achieve this functionality and so this function is required to take the
  /// output location as an input in order for the signature of the templated function to change.
  /// Otherwise it would return a vector of copied data.
  /// \param The target column name.
  /// \param A reference to a vector that will be used to store the copied data.
  virtual void getColumn(const std::string&, std::vector<int>&) const = 0;
  virtual void getColumn(const std::string&, std::vector<std::int64_t>&) const = 0;
  virtual void getColumn(const std::string&, std::vector<float>&) const = 0;
  virtual void getColumn(const std::string&, std::vector<std::string>&) const = 0;

  /// \brief Outputs the contents to screen. Used primarily for debugging and development.
  virtual void print() = 0;
};
}  // namespace osdf

#endif  // CONTAINERS_IVIEW_H_
