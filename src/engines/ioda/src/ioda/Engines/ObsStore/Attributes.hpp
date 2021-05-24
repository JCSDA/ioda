/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_internals_engines_obsstore
 *
 * @{
 * \file Attributes.hpp
 * \brief Functions for ObsStore Attribute and Has_Attributes
 */
#pragma once

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "./Selection.hpp"
#include "./Types.hpp"
#include "./VarAttrStore.hpp"

namespace ioda {
namespace ObsStore {
// Spurious warning on Intel compilers:
// https://stackoverflow.com/questions/2571850/why-does-enable-shared-from-this-have-a-non-virtual-destructor
#if defined(__INTEL_COMPILER)
#  pragma warning(push)
#  pragma warning(disable : 444)
#endif
/// \ingroup ioda_internals_engines_obsstore
class Attribute : public std::enable_shared_from_this<Attribute> {
private:
  /// \brief holds dimension sizes (vector length is rank of dimensions)
  std::vector<std::size_t> dimensions_;
  /// \brief holds ObsStore data type
  ObsTypes dtype_ = ObsTypes::NOTYPE;
  /// \brief ObsStore data type
  /// \note Unused for now.
  std::size_t dtype_size_ = 0;

  /// \brief container for attribute data values
  std::unique_ptr<VarAttrStore_Base> attr_data_;

public:
  Attribute() {}
  Attribute(const std::vector<std::size_t>& dimensions, const ObsTypes& dtype);
  ~Attribute() {}

  /// \brief returns dimensions vector
  std::vector<std::size_t> get_dimensions() const;
  /// \brief returns true if requested type matches stored type
  /// \param dtype ObsStore Type being checked
  bool isOfType(ObsTypes dtype) const;
  /// \brief returns the data type.
  inline std::pair<ObsTypes, size_t> dtype() const { return std::make_pair(dtype_, dtype_size_); }

  /// \brief transfer data into attribute
  /// \param data contiguous block of data to transfer
  /// \param dtype ObsStore Type
  std::shared_ptr<Attribute> write(gsl::span<char> data, ObsTypes dtype);
  /// \brief transfer data from attribute
  /// \param data contiguous block of data to transfer
  /// \param dtype ObsStore Type
  std::shared_ptr<Attribute> read(gsl::span<char> data, ObsTypes dtype);
};

/// \ingroup ioda_internals_engines_obsstore
class Has_Attributes {
private:
  /// \brief container of attributes
  std::map<std::string, std::shared_ptr<Attribute>> attributes_;

public:
  Has_Attributes() {}
  ~Has_Attributes() {}

  /// \brief create a new attribute
  /// \param name name of new attribute
  /// \param dtype ObsStore Type of new attribute
  /// \param dims shape of new attribute
  std::shared_ptr<Attribute> create(const std::string& name, const ioda::ObsStore::ObsTypes& dtype,
                                    const std::vector<std::size_t>& dims);

  /// \brief open an exsiting attribute (throws exception if not found)
  /// \param name name of attribute
  std::shared_ptr<Attribute> open(const std::string& name) const;

  /// \brief returns true if attribute is in the container
  /// \param name name of attribute
  bool exists(const std::string& name) const;

  /// \brief remove attribtute from container
  /// \param name name of attribute
  void remove(const std::string& name);

  /// \brief rename attribtute in container
  /// \param oldName current name of attribute
  /// \param newName new name for attribute
  void rename(const std::string& oldName, const std::string& newName);

  /// \brief returns a list of the names of attributes in the container
  std::vector<std::string> list() const;
};
#if defined(__INTEL_COMPILER)
#  pragma warning(pop)
#endif
}  // namespace ObsStore
}  // namespace ioda

/// @}
