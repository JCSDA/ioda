#pragma once
/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_cxx_layout
 *
 * @{
 * \file Layout.h
 * \brief Contains definitions for how data are arranged in ioda internally.
 */

#include <memory>
#include <string>
#include <typeindex>
#include <utility>

#include "ioda/defs.h"

namespace ioda {
class Group;

namespace detail {
class Group_Base;
class Group_Backend;
class DataLayoutPolicy;

/// \brief Policy used for setting locations for Variable access
/// \ingroup ioda_cxx_layout
/// \note Using std::enable_shared_from_this as part of the pybind11 interface.
///   We pass this to ObsGroup as a shared_ptr.
///   See https://pybind11.readthedocs.io/en/stable/advanced/smart_ptrs.html#std-shared-ptr
class IODA_DL DataLayoutPolicy : public std::enable_shared_from_this<DataLayoutPolicy> {
public:
  virtual ~DataLayoutPolicy();
  enum class Policies {
    /// Do no manipulation of the Group / Variable layout.
    None,
    /// Transform "Variable@Group" into "Group/Variable". Ensure that
    ///   group names match a few predefined keys.
    ObsGroup,
    /// Uses an auxiliary YAML dictionary to convert ODB variable/group naming conventions to
    ///  IODA equivalents. Transform "Variable@Group" into "Group/Variable". Ensure that
    ///  the new group names match a few predefined keys.
    ObsGroupODB
  };
  enum class MergeMethod {
    /// Concatenate complementary variables entry-by-entry
    Concat
  };

  /// Factory generator.
  static std::shared_ptr<const DataLayoutPolicy> generate(const std::string &polid = "");
  /// Factory generator (ODB-specific)
  /// \p mapPath path to a yaml file that defines how input file variables should be renamed
  ///   upon import to ioda.
  static std::shared_ptr<const DataLayoutPolicy> generate(const std::string &polid,
                                                          const std::string &mapPath);
  /// Factory generator.
  static std::shared_ptr<const DataLayoutPolicy> generate(Policies pol = Policies::None);
  /// Factory generator (ODB-specific)
  /// \p mapPath path to a yaml file that defines how input file variables should be renamed
  ///   upon import to ioda.
  static std::shared_ptr<const DataLayoutPolicy> generate(Policies pol,
                                                          const std::string &mapPath);
  /// \internal pybind11 overload casts do not work on some compilers.
  static inline std::shared_ptr<const DataLayoutPolicy> _py_generate1(const std::string &polid) {
    return generate(polid);
  }
  /// \internal pybind11 overload casts do not work on some compilers.
  static inline std::shared_ptr<const DataLayoutPolicy> _py_generate2(Policies pol) {
    return generate(pol);
  }

  /// Create default groups and write default attributes upon object
  /// creation / initialization.
  virtual void initializeStructure(Group_Base &) const;
  /// \brief Map a user-specified Variable path to the correct location.
  /// \details This allows us to keep the frontend paths consistent, and we
  ///   can instead do a path transformation to hide implementation details
  ///   from end users.
  ///
  /// The default policy is to pass paths expressed with forward slashes ("MetaData/Longitude")
  /// unchanged. If we pass paths using '@' notation, then reverse the path component
  /// (i.e. "TB@ObsValue" becomes "ObsValue/TB").
  /// \note We can apply these policies in both the frontend and inside of the engines.
  /// \param inStr is the user-provided string. Ex: "TB@ObsValue" or "MetaData/Latitude",
  ///   or even a fundamental dimension ("ChannelNumber").
  /// \returns A canonical path, always of the form "Group/Variable". In case of a dimension
  ///   scale, then there is no group name, but for every other Variable, there is a Group name.
  virtual std::string doMap(const std::string &) const;

  /// Check if the named variable will be a part of a derived variable
  virtual bool isComplementary(const std::string &) const;

  /// Check if the named variable is in the Variables section of the ODB mapping file.
  virtual bool isMapped(const std::string &) const;

  /// Returns the position of the input variable in the derived variable.
  /// \throws If the input is not part of a derived variable.
  virtual size_t getComplementaryPosition(const std::string &) const;

  /// Returns the derived variable name to be used in ioda.
  /// \throws If the input is not part of a derived variable.
  virtual std::string getOutputNameFromComponent(const std::string &) const;

  /// Returns the data type of the derived variable.
  /// \throws If the input is not part of a derived variable.
  virtual std::type_index getOutputVariableDataType(const std::string &) const;

  /// Returns the merge method for derived variables.
  /// \throws If the input is not part of a derived variable.
  virtual MergeMethod getMergeMethod(const std::string &) const;

  /// Returns the count of input variables needed.
  /// \throws If the input is not part of a derived variable.
  virtual size_t getInputsNeeded(const std::string &) const;

  /// Returns the variable's unit if it has been specified.
  /// \returns A pair of (found, unit) indicating if a unit was found and what it is.
  /// \throws If the input is not listed in Variables section of mapping file.
  virtual std::pair<bool, std::string> getUnit(const std::string &) const;

  /// A descriptive name for the policy.
  virtual std::string name() const;

  DataLayoutPolicy();
};
}  // namespace detail
}  // namespace ioda

/// @}
