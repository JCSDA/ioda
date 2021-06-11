/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \defgroup ioda_internals_engines_obsstore ObsStore Engine
 * \brief Implementation of the in-memory ObsStore backend.
 * \ingroup ioda_internals_engines
 *
 * @{
 * \file Group.hpp
 * \brief Functions for ObsStore Group and Has_Groups
 */
#pragma once

#include <exception>
#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "./Attributes.hpp"

namespace ioda {
namespace ObsStore {
class Has_Variables;
/// \ingroup ioda_internals_engines_obsstore
class Group {
private:
  /// \brief container for child groups
  std::map<std::string, std::shared_ptr<Group>> child_groups_;

  /// \brief split a path into the first level and remainder of the path
  /// \param path Hierarchical path
  static std::vector<std::string> splitFirstLevel(const std::string& path);

public:
  Group();
  virtual ~Group();

  /// \brief container for attributes
  std::shared_ptr<Has_Attributes> atts;

  /// \brief container for variables
  std::shared_ptr<Has_Variables> vars;

  /// \brief List all groups under this group
  std::list<std::string> list() const;

  /// \brief List child objects
  /// \param filter is a filter for the search
  /// \param recurse turns off / on recursion
  /// \param res are the search results.
  /// \param prefix is used with recursion. Start with "".
  ///   Each new element gets a new "group/" prefix added.
  void listObjects(ObjectType filter, bool recurse,
                   std::map<ObjectType, std::list<std::string>>& res,
                   const std::string& prefix = "") const;

  /// \brief returns true if child group exists
  /// \param name of child group
  bool exists(const std::string& name);

  /// \brief create a new group
  /// \param name name of child group
  std::shared_ptr<Group> create(const std::string& name);

  /// \brief open an existing child group
  /// \param name name of child group
  std::shared_ptr<Group> open(const std::string& name, const bool throwIfNotFound = true);

  /// \brief Creates a root group
  static std::shared_ptr<Group> createRootGroup();
};
}  // namespace ObsStore
}  // namespace ioda

/// @}
