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

#include "./Type.hpp"
#include "ioda/Types/Type.h"
#include "ioda/Types/Type_Provider.h"
#include "ioda/defs.h"

namespace ioda {
namespace Engines {
namespace ObsStore {

/// \brief set of functions to return ObsStore Types
/// \ingroup ioda_internals_engines_obsstore
class IODA_DL ObsStore_Type_Provider : public detail::Type_Provider {
private:
  typedef struct ObsTypeInfo {
    ioda::ObsStore::ObsTypes type;
    ioda::ObsStore::ObsTypeClasses type_class;
    std::size_t size;
    bool is_signed;

    ObsTypeInfo(const ioda::ObsStore::ObsTypes obsType,
                const ioda::ObsStore::ObsTypeClasses obsTypeClass,
                const std::size_t obsTypeSize, const bool isSigned)
        : type(obsType), type_class(obsTypeClass), size(obsTypeSize), is_signed(isSigned) {
    }

  } ObsTypeInfo;

public:
  virtual ~ObsStore_Type_Provider();

  /// \brief convert C++ fundamental type to ObsStore Type
  /// \param type C++ fundamtentl type
  static ObsTypeInfo getFundamentalObsStoreType(std::type_index type);

  /// \brief create mapping between C++ fundamental type and ObsStore Type
  /// \param type C++ fundamtentl type
  Type makeFundamentalType(std::type_index type) const final;

  // \brief create mapping between array type and ObsStore type
  // \param dimensions sizes of dimensions (note length of list corresponds to rank)
  // \param typeOuter data type of array itself
  // \param typeInner data type of elements in the array
  Type makeArrayType(std::initializer_list<Dimensions_t> dimensions,
      std::type_index typeOuter, std::type_index typeInner) const final;

  /// \brief create mapping between ioda string type and ObsStore string type
  /// \param typeOuter C++ fundamental type of characters in the string
  /// \param string_length length, in characters, of string
  /// \param cset character set of the string (ignored)
  Type makeStringType(std::type_index typeOuter, size_t string_length, StringCSet cset) const final;

  /// \brief return owner (responsible for destructing) of memory holding strings in ObsStore
  ioda::detail::PointerOwner getReturnedPointerOwner() const final;

  /// \brief create instance of a type provider for frontend
  static ObsStore_Type_Provider* instance();
};

/// \ingroup ioda_internals_engines_obsstore
class IODA_DL ObsStore_Type : public detail::Type_Backend {
private:
  /// \brief holds ObsStore data type for mapping with corresponding ioda::Type
  std::shared_ptr<ioda::ObsStore::Type> type_;

public:
  ObsStore_Type(std::shared_ptr<ioda::ObsStore::Type> type);
  ~ObsStore_Type() {}

  /// \brief return backend data type
  const ioda::ObsStore::Type & getType() const { return *type_; }

  /// \brief return size (in bytes) of given type
  std::size_t getSize() const final { return type_->getSize(); }

  /// \brief return backend type class
  TypeClass getClass() const final;

  /// \brief return base type
  Type getBaseType() const final;

  /// \brief true if type is a signed type, such as int (as opposed to unsigned int)
  bool isTypeSigned() const final { return type_->isTypeSigned(); }

  /// \brief return backend type dimensions
  std::vector<Dimensions_t> getDimensions() const final;

};
}  // namespace ObsStore
}  // namespace Engines
}  // namespace ioda

/// @}
