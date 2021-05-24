/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_internals_engines_obsstore
 *
 * @{
 * \file ObsStore-types.h
 * \brief Functions for translating ioda::Types to ObsStore Types
 */
#pragma once
#include <utility>

#include "./Types.hpp"
#include "ioda/Types/Type.h"
#include "ioda/Types/Type_Provider.h"
#include "ioda/defs.h"

namespace ioda {
namespace Engines {
namespace ObsStore {
typedef std::pair<ioda::ObsStore::ObsTypes, std::size_t> ObsTypeInfo;

/// \brief set of functions to return ObsStore Types
/// \ingroup ioda_internals_engines_obsstore
class IODA_DL ObsStore_Type_Provider : public detail::Type_Provider {
public:
  virtual ~ObsStore_Type_Provider();

  /// \brief convert C++ fundamental type to ObsStore Type
  /// \param type C++ fundamtentl type
  static ObsTypeInfo getFundamentalObsStoreType(std::type_index type);

  /// \brief create mapping between C++ fundamental type and ObsStore Type
  /// \param type C++ fundamtentl type
  Type makeFundamentalType(std::type_index type) const final;

  // Leave makeArrayType undeclared for now. Add in later if needed.
  // Let makeArrayType method in ioda::TypeProvider handle this case
  // which will issue an error.
  // virtual Type makeArrayType(std::initializer_list<Dimensions_t> dimensions, std::type_index
  // typeOuter, std::type_index typeInner) const override final;

  /// \brief create mapping between ioda string type and ObsStore string type
  /// \param string_length length, in characters, of string
  /// \param typeOuter C++ fundamental type of characters in the string
  Type makeStringType(size_t string_length, std::type_index typeOuter) const final;

  /// \brief return owner (responsible for destructing) of memory holding strings in ObsStore
  ioda::detail::PointerOwner getReturnedPointerOwner() const final;

  /// \brief create instance of a type provider for frontend
  static ObsStore_Type_Provider* instance();
};

/// \ingroup ioda_internals_engines_obsstore
class IODA_DL ObsStore_Type : public detail::Type_Backend {
private:
  /// \brief holds ObsStore type for mapping with corresponding ioda::Type
  ioda::ObsStore::ObsTypes dtype_;

  /// \brief size in bytes of stored type
  std::size_t dtype_size_;

public:
  ObsStore_Type(ObsTypeInfo& h);
  virtual ~ObsStore_Type();

  /// \brief holds ObsStore type for mapping with corresponding ioda::Type
  ioda::ObsStore::ObsTypes dtype() const;

  /// \brief return size (in bytes) of given type
  std::size_t dtype_size() const;
  size_t getSize() const final;
};
}  // namespace ObsStore
}  // namespace Engines
}  // namespace ioda

/// @}
