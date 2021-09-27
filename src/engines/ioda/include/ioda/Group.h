#pragma once
/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \defgroup ioda_cxx_layout Groups and Data Layout
 * \brief Public API for ioda::Group, ioda::ObsGroup, and data layout policies.
 * \ingroup ioda_cxx_api
 *
 * @{
 * \file Group.h
 * \brief Interfaces for ioda::Group and related classes.
 */

#include <gsl/gsl-lite.hpp>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "ioda/Attributes/Has_Attributes.h"
#include "ioda/Types/Has_Types.h"
#include "ioda/Variables/FillPolicy.h"
#include "ioda/Variables/Has_Variables.h"
#include "ioda/defs.h"

namespace ioda {
class Group;

namespace Engines {
struct Capabilities;
}  // namespace Engines

namespace detail {
class Group_Base;
class Group_Backend;

/// \brief Hidden base class to prevent constructor confusion.
/// \ingroup ioda_cxx_layout
/// \note enable_shared_from_this is a hint to pybind11.
class IODA_DL Group_Base {
  std::shared_ptr<Group_Backend> backend_;

protected:
  Group_Base(std::shared_ptr<Group_Backend>);

public:
  virtual ~Group_Base();

  std::shared_ptr<Group_Backend> getBackend() const { return backend_; }

  /// Get capabilities of the Engine backing this Group
  virtual ::ioda::Engines::Capabilities getCapabilities() const;

  /// \brief Get the fill value policy used for Variables within this Group
  /// \details The backend has to be consulted for this operation. Storage of this policy is
  ///   backend-dependent.
  virtual FillValuePolicy getFillValuePolicy() const;

  /// \brief List all one-level child groups in this group
  /// \details This function exists to provide the same calling semantics as
  ///   vars.list() and atts.list(). It is useful for human exploration of the contents
  ///   of a Group.
  /// \returns A vector of strings listing the child groups. The strings are unordered.
  /// \see listObjects if you need to enumerate both Groups and Variables, or
  ///   if you want to do a recursive search. This function is more sophisticated, and
  ///   depending on the backend it may be faster than recursive calls to
  ///   list() and vars.list().
  std::vector<std::string> list() const;
  /// Same as list(). Uniform semantics with atts() and vars().
  inline std::vector<std::string> groups() const { return list(); }

  /// \brief List all objects (groups + variables) within this group
  /// \param recurse indicates whether the search should be one-level or recursive. If
  ///   multiple possible paths exist for an object, only one is actually returned.
  /// \param filter allows you to search for only a certain type of object, such as a
  ///   Group or Variable.
  /// \returns a map of ObjectTypes. Each mapping contains a vector of strings indicating
  ///   object names. If a filter is provided, then this map only contains the filtered
  ///   object type. Otherwise, the map always has vectors for each element of ObjectType
  ///   (Group, Variable, etc.), although these vectors may be empty if no object of a
  ///   certain type is found.
  /// \todo This function should list all *distinct* objects. In the future, this will mean
  ///   that 1) hard-linked duplicate objects will only be listed once, 2) soft
  ///   links pointing to the same object will only be traversed once, and
  ///   3) multiple external links to the same object will only be traversed once.
  ///   If you would want to list *indistinct* objects, then there will be a
  ///   listLinks function.
  /// \todo Once links are implemented, add an option to auto-resolve
  ///   soft and external links.
  virtual std::map<ObjectType, std::vector<std::string>> listObjects(ObjectType filter
                                                                     = ObjectType::Ignored,
                                                                     bool recurse = false) const;

  template <ObjectType objectClass>
  std::vector<std::string> listObjects(bool recurse = false) const {
    return listObjects(objectClass, recurse)[objectClass];
  }

  /// Does a group exist at the specified path?
  /// \param name is the group name.
  /// \returns true if a group does exist.
  /// \return false if a group does not exist.
  virtual bool exists(const std::string& name) const;

  /// \brief Create a group
  /// \param name is the group name.
  /// \returns an invalid handle on failure. (i.e. return.isGroup() == false).
  /// \returns a scoped handle to the group on success.
  virtual Group create(const std::string& name);

  /// \brief Open a group
  /// \returns an invalid handle if an error occurred. (i.e. return.isGroup() == false).
  /// \returns a scoped handle to the group upon success.
  /// \note It is possible to have multiple handles opened for the group
  /// simultaneously.
  /// \param name is the name of the child group to open.
  virtual Group open(const std::string& name) const;

  /// Use this to access the metadata for the group / ObsSpace.
  Has_Attributes atts;

  /// Use this to access named data types.
  Has_Types types;

  /// Use this to access variables
  Has_Variables vars;
};

class IODA_DL Group_Backend : public Group_Base {
public:
  virtual ~Group_Backend();
  /// Default fill value policy is NETCDF4. Overridable on a per-backend basis.
  FillValuePolicy getFillValuePolicy() const override;

  /// @}

protected:
  Group_Backend();
};

}  // namespace detail

/** \brief Groups are a new implementation of ObsSpaces.
 * \ingroup ioda_cxx_layout
 *
 * \see \ref Groups for a examples.
 *
 * A group can be thought of as a folder that contains Variables and Metadata.
 * A group can also contain child groups, allowing for our ObsSpaces to exist in a
 * nested tree-like structure, which removes the need for having an ObsSpaceContainer.
 *
 * Groups are implemented in several backends, such as the in-memory IODA store, the HDF5 disk
 * backend, the HDF5 in-memory backend, the ATLAS backend, et cetera. The root Group is mounted
 * using one of these backends (probably as a File object, which is a special type of Group),
 * and additional backends may be mounted into the tree structure.
 *
 * \throws std::logic_error if the backend handle points to something other than a group.
 * \throws std::logic_error if the backend handle is invalid.
 * \throws std::logic_error if any error has occurred.
 * \author Ryan Honeyager (honeyage@ucar.edu)
 **/
class IODA_DL Group : public detail::Group_Base {
public:
  Group();
  Group(std::shared_ptr<detail::Group_Backend>);
  virtual ~Group();
};

}  // namespace ioda

/// @}
