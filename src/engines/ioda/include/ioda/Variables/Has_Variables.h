#pragma once
/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_cxx_variable
 *
 * @{
 * \file Has_Variables.h
 * \brief Interfaces for ioda::Has_Variables and related classes.
 */

#include <cstring>
#include <gsl/gsl-lite.hpp>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "ioda/Attributes/Attribute_Creator.h"
#include "ioda/Exception.h"
#include "ioda/Layout.h"
#include "ioda/Misc/Eigen_Compat.h"
#include "ioda/Misc/MergeMethods.h"
#include "ioda/Types/Type.h"
#include "ioda/Variables/FillPolicy.h"
#include "ioda/Variables/Variable.h"
#include "ioda/defs.h"

namespace ioda {
class Has_Variables;
class ObsGroup;
struct Named_Variable;
namespace detail {
class Has_Variables_Backend;
class Has_Variables_Base;
class DataLayoutPolicy;
class Group_Base;

}  // namespace detail

/// \brief A few chunking strategies for Variables
namespace chunking {
/// Convenience function for setting default chunking parameters.
inline bool Chunking_Max(const std::vector<Dimensions_t>& in, std::vector<Dimensions_t>& out) {
  out = in;
  return true;
}
}  // namespace chunking

/// \brief Used to specify Variable creation-time properties.
/// \ingroup ioda_cxx_variable
struct IODA_DL VariableCreationParameters {
private:
  std::vector<std::pair<unsigned int, Variable> > dimsToAttach_;
  std::string dimScaleName_;

public:
  /// @name Fill Value
  /// @{

  detail::FillValueData_t fillValue_;

  template <class DataType>
  VariableCreationParameters& setFillValue(DataType fill) {
    detail::assignFillValue<DataType>(fillValue_, fill);
    return *this;
  }
  inline VariableCreationParameters& unsetFillValue() {
    fillValue_.set_ = false;
    return *this;
  }

  /// @}
  /// @name Chunking and compression
  /// @{

  /// \brief Do we chunk this variable? Required for extendible / compressible Variables.
  /// \details Requires a chunking strategy.
  bool chunk = false;

  /// \brief Manually specify the chunks. Never directly use. Use getChunks(...) instead.
  std::vector<Dimensions_t> chunks;
  /// Set variable chunking strategy. Used only if chunk == true and chunks.size() == 0.
  std::function<bool(const std::vector<Dimensions_t>&, std::vector<Dimensions_t>&)>
    fChunkingStrategy = chunking::Chunking_Max;
  /// Figure out the chunking size
  /// \param cur_dims are the current dimensions
  std::vector<Dimensions_t> getChunks(const std::vector<Dimensions_t>& cur_dims) const {
    if (chunks.size()) return chunks;
    std::vector<Dimensions_t> res;
    if (fChunkingStrategy(cur_dims, res)) return res;
    throw Exception("Cannot figure out an appropriate chunking size.", ioda_Here());
  }

  bool gzip_                        = false;
  bool szip_                        = false;
  int gzip_level_                   = 6;  // 1 (fastest) - 9 (most compression)
  unsigned int szip_PixelsPerBlock_ = 16;
  unsigned int szip_options_        = 4;  // Defined as H5_SZIP_EC_OPTION_MASK in hdf5.h;

  void noCompress();
  void compressWithGZIP(int level = 6);
  void compressWithSZIP(unsigned PixelsPerBlock = 16, unsigned options = 4);

  /// @}
  /// @name General Functions
  /// @{

  /// Set any initial attributes here
  Attribute_Creator_Store atts;

  VariableCreationParameters();
  VariableCreationParameters(const VariableCreationParameters&);
  VariableCreationParameters& operator=(const VariableCreationParameters&);

  template<class DataType>
  static VariableCreationParameters defaulted() {
    VariableCreationParameters ret;
    ret.chunk = true;
    ret.compressWithGZIP();
    FillValuePolicies::applyFillValuePolicy<DataType>(FillValuePolicy::NETCDF4, ret.fillValue_);
    return ret;
  }
  template <class DataType>
  static VariableCreationParameters defaults() {
    return defaulted<DataType>();
  }

  /// Finalize routine to make sure struct members are intact (e.g. for fill values)
  detail::FillValueData_t::FillValueUnion_t finalize() const { return fillValue_.finalize(); }

  detail::python_bindings::VariableCreationFillValues<VariableCreationParameters> _py_setFillValue;

private:
  friend class detail::Has_Variables_Base;
  /// Apply the properties to a Variable (second pass; after Variable is created).
  Variable applyImmediatelyAfterVariableCreation(Variable h) const;

  /// @}
};

typedef std::vector<Variable> NewVariables_Scales_t;
/// \brief Used to specify a new variable with the collective createWithScales function.
struct IODA_DL NewVariable_Base : std::enable_shared_from_this<NewVariable_Base> {
  /// Name of the variable.
  std::string name_;
  /// Type of the new dimension. Int, char, etc. Used if a type is not passed directly.
  std::type_index dataType_;
  /// Type of the new dimension. Used if a type is passed directly.
  Type dataTypeKnown_;
  /// Dimension scales
  NewVariables_Scales_t scales_;
  /// Var creation params
  VariableCreationParameters vcp_;

  virtual ~NewVariable_Base() {}

  NewVariable_Base(const std::string& name, const Type& dataType,
                   const NewVariables_Scales_t& scales,
                   const VariableCreationParameters& params)
      : name_(name), dataType_(typeid(void)), dataTypeKnown_(dataType),
        scales_(scales),
        vcp_(params) {}

  NewVariable_Base(const std::string& name, const std::type_index& dataType,
                   const NewVariables_Scales_t& scales,
                   const VariableCreationParameters& params)
      : name_(name),
        dataType_(dataType),
        scales_(scales),
        vcp_(params) {}
};
typedef std::vector<std::shared_ptr<NewVariable_Base>> NewVariables_t;

template <class DataType>
inline std::shared_ptr<NewVariable_Base> NewVariable(
  const std::string& name, const NewVariables_Scales_t& scales,
  const VariableCreationParameters& params = VariableCreationParameters::defaulted<DataType>()) {
  return std::make_shared<NewVariable_Base>(name, typeid(DataType), scales, params);
}

inline std::shared_ptr<NewVariable_Base> NewVariable(const std::string& name,
                                                     const Type& DataType,
                                                     const NewVariables_Scales_t& scales,
                                                     const VariableCreationParameters& params
                                                     = VariableCreationParameters()) {
  return std::make_shared<NewVariable_Base>(name, DataType, scales, params);
}


namespace detail {

/// \ingroup ioda_cxx_variable
class IODA_DL Has_Variables_Base {
  // friend class Group_Base;
private:
  /// Using an opaque object to implement the backend.
  std::shared_ptr<Has_Variables_Backend> backend_;
  /// Set by ObsGroup.
  std::shared_ptr<const detail::DataLayoutPolicy> layout_;
  std::vector<ComplementaryVariableCreationParameters> complementaryVariables_;
  /// \brief FillValuePolicy helper
  /// \details Hides the template function calls, so that the headers are smaller.
  static void _py_fvp_helper(BasicTypes dataType, FillValuePolicy& fvp,
                             VariableCreationParameters& params);

  ComplementaryVariableCreationParameters createDerivedVariableParameters(
      const std::string &inputName, const std::string &outputName, size_t position);
  std::vector<std::vector<std::string>> loadComponentVariableData(
      const ComplementaryVariableCreationParameters& derivedVariableParams);

protected:
  Has_Variables_Base(std::shared_ptr<Has_Variables_Backend>,
                     std::shared_ptr<const DataLayoutPolicy> = nullptr);

public:
  virtual ~Has_Variables_Base();

  /// Set the mapping policy to determine the Layout of Variables stored under this Group.
  /// Usually only set by ObsGroup when we create / open.
  virtual void setLayout(std::shared_ptr<const detail::DataLayoutPolicy>);

  /// Query the backend and get the type provider.
  virtual Type_Provider* getTypeProvider() const;

  /// \brief Get the fill value policy used for Variables within this Group
  /// \details The backend has to be consulted for this operation. Storage of this policy is
  /// backend-dependent.
  virtual FillValuePolicy getFillValuePolicy() const;

  /// @name General Functions
  /// @{
  ///

  /// \brief Does a Variable with the specified name exist?
  /// \param name is the name of the Variable that we are looking for.
  /// \returns true if it exists.
  /// \returns false otherwise.
  virtual bool exists(const std::string& name) const;
  /// \brief Delete an Attribute with the specified name.
  /// \param attname is the name of the Variable that we are deleting.
  /// \throws ioda::Exception if no such attribute exists.
  virtual void remove(const std::string& name);
  /// \brief Open a Variable by name
  /// \param name is the name of the Variable to be opened.
  /// \returns An instance of a Variable that can be queried (with getDimensions()) and read.
  virtual Variable open(const std::string& name) const;
  /// \brief Open a Variable by name
  /// \param name is the name of the Variable to be opened.
  /// \returns An instance of a Variable that can be queried (with getDimensions()) and read.
  inline Variable operator[](const std::string& name) const { return open(name); }

  /// List all Variables under this group (one-level search).
  /// \see Group_Base::listObjects if you need to enumerate both Groups and Variables, or
  ///   if you need recursion.
  virtual std::vector<std::string> list() const;
  /// Convenience function to list all Variables under this group (one-level search).
  /// \see Group_Base::listObjects if you need to enumerate both Groups and Variables, or
  ///   if you need recursion.
  inline std::vector<std::string> operator()() const { return list(); }

  /// \brief Combines all complementary variables as specified in the mapping file, opens them,
  /// and optionally removes the originals from the ObsGroup.
  ///
  /// \p removeOriginals determines if the original complementary variables should be removed from
  /// the ObsGroup. Later functionality will ensure that the original complementary variables can
  /// be recreated on writing back to the original file.
  void stitchComplementaryVariables(bool removeOriginals = true);

  /// \brief Converts unit to SI for all eligible variables. If conversion function not defined,
  /// stores unit as attribute.
  ///
  /// Makes the conversion if the variable's unit is defined in the mapping file and the unit conversion
  /// is defined in UnitConversions.h.
  void convertVariableUnits(std::ostream &out = std::cerr);

  /// \brief Create a Variable without setting its data.
  /// \param attrname is the name of the Variable.
  /// \param dimensions is a vector representing the size of the metadata.
  ///   Each element of the vector is a dimension with a certain size.
  /// \param in_memory_datatype is the runtime description of the Attribute's data type.
  /// \returns A Variable that can be written to.
  virtual Variable create(const std::string& name, const Type& in_memory_dataType,
                          const std::vector<Dimensions_t>& dimensions     = {1},
                          const std::vector<Dimensions_t>& max_dimensions = {},
                          const VariableCreationParameters& params = VariableCreationParameters());

  /// Python compatability function
  /// \note Multiple ways to specify dimensions to match possible
  ///   Python function signatures.
  Variable _create_py(const std::string& name, BasicTypes dataType,
                             const std::vector<Dimensions_t>& cur_dimensions = {1},
                             const std::vector<Dimensions_t>& max_dimensions = {},
                             const std::vector<Variable>& dimension_scales   = {},
                             const VariableCreationParameters& params
                             = VariableCreationParameters());

  inline Variable create(const std::string& name, const Type& in_memory_dataType,
                         const ioda::Dimensions& dims,
                         const VariableCreationParameters& params = VariableCreationParameters()) {
    return create(name, in_memory_dataType, dims.dimsCur, dims.dimsMax, params);
  }

  /// \brief Create a Variable without setting its data.
  /// \tparam DataType is the type of the data. I.e. float, int32_t, uint16_t, std::string, etc.
  /// \param name is the name of the Variable.
  /// \param dimensions is a vector representing the size of the metadata. Each element of the
  ///   vector is a dimension with a certain size.
  /// \returns A Variable that can be written to.
  template <class DataType>
  Variable create(const std::string& name, const std::vector<Dimensions_t>& dimensions = {1},
                  const std::vector<Dimensions_t>& max_dimensions = {},
                  const VariableCreationParameters& params        = VariableCreationParameters::defaulted<DataType>()) {
    try {
      VariableCreationParameters params2 = params;
      FillValuePolicies::applyFillValuePolicy<DataType>(getFillValuePolicy(), params2.fillValue_);
      Type in_memory_dataType = Types::GetType<DataType>(getTypeProvider());
      auto var                = create(name, in_memory_dataType, dimensions,
        max_dimensions, params2);
      return var;
    } catch (...) {
      std::throw_with_nested(Exception(ioda_Here()));
    }
  }

  template <class DataType>
  Variable create(const std::string& name, const ioda::Dimensions& dims,
                  const VariableCreationParameters& params
                  = VariableCreationParameters::defaulted<DataType>()) {
    try {
      VariableCreationParameters params2 = params;
      FillValuePolicies::applyFillValuePolicy<DataType>(getFillValuePolicy(), params2.fillValue_);
      return create<DataType>(name, dims.dimsCur, dims.dimsMax, params2);
    } catch (...) {
      std::throw_with_nested(Exception(ioda_Here()));
    }
  }

  /// \brief Convenience function to create a Variable from certain dimension scales.
  /// \tparam DataType is the type of the data. I.e. int, int32_t, uint16_t, std::string, etc.
  /// \param name is the name of the Variable.
  /// \param dimensions is a vector representing the size of the metadata. Each element of the
  ///   vector is a dimension with a certain size.
  /// \returns A Variable that can be written to.
  template <class DataType>
  Variable createWithScales(const std::string& name,
                            const std::vector<Variable>& dimension_scales,
                            const VariableCreationParameters& params
                            = VariableCreationParameters::defaulted<DataType>()) {
    try {
      Type in_memory_dataType = Types::GetType<DataType>(getTypeProvider());

      NewVariables_t newvars{NewVariable(name, in_memory_dataType, dimension_scales, params)};
      createWithScales(newvars);
      return open(name);
    } catch (...) {
      std::throw_with_nested(Exception(ioda_Here()));
    }
  }

  /// @brief Collective function optimized to mass-construct variables and attach scales.
  /// @param newvars is a vector of the new variables to be created.
  /// @see NewVariable for the signature of the objects to add.
  void createWithScales(const NewVariables_t& newvars);

  /// @}
  /// @name Collective functions
  /// @brief These functions apply the an operation to a *set* of variables in situations where
  ///   such an operation would produce better performance results than a loop of serial
  ///   function calls.
  /// @{
  
  /// @brief Attach dimension scales to many Dimension Numbers in a set of Variables.
  /// @param DimensionNumber 
  /// @param mapping is the scale mappings for each variable. The first part of the pair refers
  ///   to the variable that you are attaching scales to. The second part is a sequence of
  ///   scales that are attached along each dimension (indexed by the vector).
  /// @details
  /// For some backends, particularly HDF5, attaching a dimension scale to a variable is a slow
  /// procedure when you have many variables. This function batches low-level calls and avoids
  /// loops.
  virtual void attachDimensionScales(
    const std::vector<std::pair<Variable, std::vector<Variable>>>& mapping);

  /// @}
};

class IODA_DL Has_Variables_Backend : public detail::Has_Variables_Base {
protected:
  Has_Variables_Backend();

public:
  virtual ~Has_Variables_Backend();
  FillValuePolicy getFillValuePolicy() const override;
  void attachDimensionScales(
    const std::vector<std::pair<Variable, std::vector<Variable>>>& mapping) override;
};
}  // namespace detail

/// \brief This class exists inside of ioda::Group and provides the interface to manipulating
///   Variables.
/// \ingroup ioda_cxx_variable
///
/// \note It should only be constructed inside of a Group. It has no meaning elsewhere.
/// \see ioda::Variable for the class that represents individual variables.
/// \throws ioda::Exception on all exceptions.
class IODA_DL Has_Variables : public detail::Has_Variables_Base {
public:
  virtual ~Has_Variables();
  Has_Variables();
  Has_Variables(std::shared_ptr<detail::Has_Variables_Backend>,
                std::shared_ptr<const detail::DataLayoutPolicy> = nullptr);
};
}  // namespace ioda

/// @}
