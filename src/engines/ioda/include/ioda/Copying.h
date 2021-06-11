#pragma once
/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file Copying.h
/// \brief Generic copying facility
/// \note Feature is under development. This is a placeholder header file.

#include <algorithm>
#include <gsl/gsl-lite.hpp>
#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "ioda/Group.h"
#include "ioda/defs.h"

namespace ioda {
class Group;
class Has_Variables;

class ObjectSelection;
struct ScaleMapping;

/// \todo This function is not yet complete.
///   It exists in its present form to provide the guts for
///   a timing test. Do not use.
/// \brief Generic data copying function
/// \param from contains the objects that we are copying
/// \param to contains the destination(s) of the copy
/// \param scale_map contains settings regarding how dimension scales
///   are propagated
IODA_DL void copy(const ObjectSelection& from, ObjectSelection& to, const ScaleMapping& scale_map);

/// \brief Allows you to select objects for a copy operation
class IODA_DL ObjectSelection {
  // friend IODA_DL void copy(const ObjectSelection&, ObjectSelection&, const ScaleMapping&);
public:
  Group g_;
  bool recurse_ = false;

public:
  ~ObjectSelection();
  ObjectSelection();
  ObjectSelection(const Group& g, bool recurse = true);
  /*
  ObjectSelection(const Variable& v) : ObjectSelection() { insert(v); }
  ObjectSelection(const std::vector<Variable>& v) : ObjectSelection() { insert(v); }
  ObjectSelection(const Has_Variables& v) : ObjectSelection() { insert(v); }
  ObjectSelection(const Group& g, const std::vector<std::string>& v) : ObjectSelection() {
  insert(g,v); }

  void insert(const ObjectSelection&);
  void insert(const Variable&);
  void insert(const std::vector<Variable>&);
  void insert(const Has_Variables&);
  void insert(const Group&, const std::vector<std::string>&);
  void insert(const Group&, bool recurse = true);

  ObjectSelection operator+(const ObjectSelection&) const;
  ObjectSelection operator+(const Variable&) const;
  ObjectSelection operator+(const std::vector<Variable>&) const;
  ObjectSelection operator+(const Has_Variables&) const;
  // Group insertions need to be wrapped by an ObjectSelection.

  ObjectSelection& operator+=(const ObjectSelection&);
  ObjectSelection& operator+=(const Variable&);
  ObjectSelection& operator+=(const std::vector<Variable>&);
  ObjectSelection& operator+=(const Has_Variables&);
  */
};

/// \brief Settings for how to remap dimension scales
struct IODA_DL ScaleMapping {
public:
  ~ScaleMapping();

  std::vector<std::pair<Variable, Variable>> map_from_to;
  std::vector<Variable> map_new;
  bool autocreate = false;
};

}  // namespace ioda
