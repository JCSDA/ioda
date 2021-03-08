/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_internals_engines_obsstore
 *
 * @{
 * \file Group.cpp
 * \brief Functions for ObsStore Group and Has_Groups
 */
#include "./Group.hpp"

#include <stdexcept>

#include "./Variables.hpp"
#include "ioda/defs.h"

namespace ioda {
namespace ObsStore {
Group::Group()
    : atts(std::make_shared<Has_Attributes>()), vars(std::make_shared<Has_Variables>()) {}
Group::~Group() = default;

std::list<std::string> Group::list() const {
  std::list<std::string> childList;
  for (auto igrp = child_groups_.begin(); igrp != child_groups_.end(); ++igrp) {
    childList.push_back(igrp->first);
  }
  return childList;
}

void Group::listObjects(ObjectType filter, bool recurse,
                        std::map<ObjectType, std::list<std::string>>& res,
                        const std::string& prefix) const {
  // If we either want to list groups or do any type of recursion, then we need
  // to get the one-level child groups.
  bool needGroups = filter == ObjectType::Ignored || filter == ObjectType::Group || recurse;
  bool needVars   = filter == ObjectType::Ignored || filter == ObjectType::Variable;

  if (needVars) {
    // Prepare empty list if it does not already exist
    if (!res.count(ObjectType::Variable)) res[ObjectType::Variable] = std::list<std::string>();

    // Back-insert these after prepending the prefix.
    auto oneLevelVars = vars->list();
    for (auto& v : oneLevelVars) res[ObjectType::Variable].push_back(prefix + v);
  }

  if (needGroups || recurse) {
    auto oneLevelChildren = list();

    if (needGroups) {
      // Prepare empty list if it does not already exist
      if (!res.count(ObjectType::Group)) res[ObjectType::Group] = std::list<std::string>();

      // Back-insert these after prepending the prefix.
      for (auto& v : oneLevelChildren) res[ObjectType::Group].push_back(prefix + v);
    }

    if (recurse) {
      for (const auto& child : oneLevelChildren)
        child_groups_.at(child)->listObjects(filter, recurse, res, std::string(child) + "/");
    }
  }
}

bool Group::exists(const std::string& name) {
  std::shared_ptr<Group> childGroup = open(name, false);
  return (childGroup != nullptr);
}

std::shared_ptr<Group> Group::create(const std::string& name) {
  // split name into first group and remaining children of the first group
  // ie, "a/b/c/d" -> "a", "b/c/d"
  std::vector<std::string> pathSections = splitFirstLevel(name);

  // If the child exists grab it, otherwise create it.
  std::shared_ptr<Group> childGroup;
  if (this->exists(pathSections[0])) {
    childGroup = this->open(pathSections[0]);
  } else {
    childGroup = std::make_shared<Group>();
    childGroup->vars->setParentGroup(childGroup);
    child_groups_.insert(
      std::pair<std::string, std::shared_ptr<Group>>(pathSections[0], childGroup));
  }

  // Recurse if there are more levels in the input name
  if (pathSections.size() > 1) {
    childGroup = childGroup->create(pathSections[1]);
  }

  return childGroup;
}

std::shared_ptr<Group> Group::open(const std::string& name, const bool throwIfNotFound) {
  std::shared_ptr<Group> childGroup(nullptr);

  // split name into first group and remaining children of the first group
  // ie, "a/b/c/d" -> "a", "b/c/d"
  std::vector<std::string> pathSections = splitFirstLevel(name);

  auto igrp = child_groups_.find(pathSections[0]);
  if (igrp == child_groups_.end()) {
    childGroup = nullptr;
  } else {
    childGroup = igrp->second;
  }

  // Recurse if there are more levels in the input name
  if ((pathSections.size() > 1) && (childGroup != nullptr)) {
    childGroup = childGroup->open(pathSections[1]);
  }

  if (throwIfNotFound && (childGroup == nullptr)) {
    throw;  // jedi_throw.add("Reason", "Child group not found");
  }

  return childGroup;
}

std::shared_ptr<Group> Group::createRootGroup() {
  std::shared_ptr<Group> group = std::make_shared<Group>();
  group->vars->setParentGroup(group);
  return group;
}

// Private methods
std::vector<std::string> Group::splitFirstLevel(const std::string& path) {
  std::vector<std::string> pathSections;
  auto pos = path.find('/');
  pathSections.push_back(path.substr(0, pos));
  if (pos != std::string::npos) {
    pathSections.push_back(path.substr(pos + 1));
  }
  return pathSections;
}
}  // namespace ObsStore
}  // namespace ioda

/// @}
