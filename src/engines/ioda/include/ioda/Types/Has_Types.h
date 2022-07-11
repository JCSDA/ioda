#pragma once
/*
 * (C) Copyright 2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_cxx_types
 *
 * @{
 * \file Has_Types.h
 * \brief Interfaces for ioda::Has_Types and related classes.
 */

#include <memory>
#include <string>
#include <vector>

#include "ioda/Types/Type.h"
#include "ioda/defs.h"

namespace ioda {
namespace detail {
class Has_Types_Backend;
class Has_Types_Base;

}  // namespace detail

namespace detail {

/// \ingroup ioda_cxx_types
class IODA_DL Has_Types_Base {
private:
  /// Using an opaque object to implement the backend.
  std::shared_ptr<Has_Types_Backend> backend_;

protected:
  Has_Types_Base(std::shared_ptr<Has_Types_Backend>);

public:
  virtual ~Has_Types_Base();

  /// Query the backend and get the type provider.
  virtual Type_Provider* getTypeProvider() const;

  /// @name General Functions
  /// @{
  ///

  /// \brief Does a Type with the specified name exist?
  /// \param name is the name of the Type that we are looking for.
  /// \returns true if it exists.
  /// \returns false otherwise.
  virtual bool exists(const std::string& name) const;
  /// \brief Delete a Type with the specified name.
  /// \param name is the name of the Type that we are deleting.
  /// \throws ioda::Exception if no such Type exists.
  virtual void remove(const std::string& name);
  /// \brief Open a Type by name
  /// \param name is the name of the Type to be opened.
  /// \returns An instance of the named Type.
  virtual Type open(const std::string& name) const;
  /// \brief Open a Type by name
  /// \param name is the name of the Type to be opened.
  /// \returns An instance of the named Type.
  inline Type operator[](const std::string& name) const { return open(name); }

  /// List all Types under this group (one-level search).
  /// \see Group_Base::listObjects if you need recursion or an otherwise better search.
  virtual std::vector<std::string> list() const;
  /// Convenience function to list all Types under this group (one-level search).
  /// \see Group_Base::listObjects if you need recursion or an otherwise better search.
  inline std::vector<std::string> operator()() const { return list(); }

  // No creation functions for now. Those can be linked into the Type_Provider, which
  // is already available via the getTypeProvider() method.

  /// @}
};

class IODA_DL Has_Types_Backend : public detail::Has_Types_Base {
protected:
  Has_Types_Backend();

public:
  virtual ~Has_Types_Backend();
};
}  // namespace detail

/// \brief This class exists inside of ioda::Group and provides the interface to manipulating
///   Types.
/// \ingroup ioda_cxx_types
///
/// \note It should only be constructed inside of a Group. It has no meaning elsewhere.
/// \see ioda::Type for the class that represents individual variables.
/// \throws ioda::Exception on all exceptions.
class IODA_DL Has_Types : public detail::Has_Types_Base {
public:
  virtual ~Has_Types();
  Has_Types();
  Has_Types(std::shared_ptr<detail::Has_Types_Backend>);
};
}  // namespace ioda

/// @}
