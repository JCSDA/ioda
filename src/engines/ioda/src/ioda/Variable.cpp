/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "ioda/Variables/Variable.h"
#include "ioda/Variables/Has_Variables.h"
#include "ioda/Exception.h"

namespace ioda {
namespace detail {

template <>
Variable_Base<>::Variable_Base(std::shared_ptr<Variable_Backend> backend)
    : backend_(backend), atts((backend) ? backend->atts : Has_Attributes()) {}
template <>
Variable_Base<>::~Variable_Base() = default;

template <>
std::shared_ptr<Variable_Backend> Variable_Base<>::get() const {
  return backend_;
}

template <>
bool Variable_Base<>::isA(Type lhs) const {
  try {
    if (backend_ == nullptr)
      throw Exception("Missing backend or unimplemented backend function.", ioda_Here());
    return backend_->isA(lhs);
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while checking variable type.", ioda_Here()));
  }
}
template <>
detail::Type_Provider* Variable_Base<>::getTypeProvider() const {
  try {
    if (backend_ == nullptr)
      throw Exception("Missing backend or unimplemented backend function.", ioda_Here());
    return backend_->getTypeProvider();
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while getting a backend type provider.", ioda_Here()));
  }
}

template <>
Type Variable_Base<>::getType() const {
  try {
    if (backend_ == nullptr)
      throw Exception("Missing backend or unimplemented backend function.", ioda_Here());
    return backend_->getType();
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while determining variable type.", ioda_Here()));
  }
}

/// \note This is very inefficient. Refactor?
template <>
BasicTypes Variable_Base<>::getBasicType() const {
  try {
    if (isA(BasicTypes::float_)) return BasicTypes::float_;
    if (isA(BasicTypes::int32_)) return BasicTypes::int32_;
    if (isA<double>()) return BasicTypes::double_;
    if (isA<int16_t>()) return BasicTypes::int16_;
    if (isA<int64_t>()) return BasicTypes::int64_;
    if (isA<uint16_t>()) return BasicTypes::uint16_;
    if (isA<uint32_t>()) return BasicTypes::uint32_;
    if (isA<uint64_t>()) return BasicTypes::uint64_;
    if (isA<std::string>()) return BasicTypes::str_;
    if (isA<long double>()) return BasicTypes::ldouble_;
    if (isA<char>()) return BasicTypes::char_;
    if (isA<bool>()) return BasicTypes::bool_;

    return BasicTypes::undefined_;
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while determining variable type.", ioda_Here()));
  }
}

template <>
bool Variable_Base<>::hasFillValue() const {
  try {
    if (backend_ == nullptr)
      throw Exception("Missing backend or unimplemented backend function.", ioda_Here());
    // In the case of the HH backend, calling the backend hasFillValue() routine will
    // consider only the hdf5 fill value property. We want to also consider the existence
    // of the netcdf _FillValue attribute.
    return (backend_->hasFillValue() || atts.exists("_FillValue"));
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while determining if a variable has a fill value.",
      ioda_Here()));
  }
}

template<>
Variable_Base<>::FillValueData_t Variable_Base<>::getNcFillValue() const {
  Variable_Base<>::FillValueData_t res;
  res.set_ = false;
  if (atts.exists("_FillValue")) {
    Attribute fvAttr = atts.open("_FillValue");

    // The runForVarType function will pass in an expression matching
    // the variable data type through the parameter typeDiscriminator.
    // runForVarType() supports more data types than are supported by
    // netcdf fill values, but it is expected to only enter this function
    // for the set of netcdf fill value types.
    runForVarType(
        [&](auto typeDiscriminator) {
           typedef decltype(typeDiscriminator) VarType;
           VarType fillVal;
           fvAttr.read<VarType>(fillVal);
           assignFillValue<VarType>(res, fillVal);
        });
  }
  return res;
}

template <>
void Variable_Base<>::checkWarnFillValue(FillValueData_t & hdfFill,
                                         FillValueData_t & ncFill) const {
  if (hdfFill.set_ && ncFill.set_) {
    // The runForVarType function will pass in an expression matching
    // the variable data type through the parameter typeDiscriminator.
    // runForVarType() supports more data types than are supported by
    // netcdf fill values, but it is expected to only enter this function
    // for the set of netcdf fill value types.
    runForVarType(
        [&](auto typeDiscriminator) {
           typedef decltype(typeDiscriminator) FillType;
           FillType hdfFillValue = ioda::detail::getFillValue<FillType>(hdfFill);
           FillType ncFillValue = ioda::detail::getFillValue<FillType>(ncFill);
           if (hdfFillValue != ncFillValue) {
             std::cout << "WARNING: ioda::Variable: hdf and netcdf fill value specifications "
                       << "do not match." << std::endl
                       << "    hdf fill value property: " << hdfFillValue << std::endl
                       << "    netcdf _FillValue attribute: " << ncFillValue << std::endl
                       << "WARNING: selecting the netcdf _FillValue attribute value: "
                       << ncFillValue << std::endl;
           }
        });
  }
}

template <>
Variable_Base<>::FillValueData_t Variable_Base<>::getFillValue() const {
  try {
    if (backend_ == nullptr)
      throw Exception("Missing backend or unimplemented backend function.", ioda_Here());
    // Need to check both the hdf5 fill value property and the netcdf _FillValue var
    // attribute. The precedence is given to the netcdf _FillValue property.
    // Issue a warning if you received fill values from both the property and attribute
    // and those two values don't match.
    Variable_Base<>::FillValueData_t res = backend_->getFillValue();
    Variable_Base<>::FillValueData_t ncRes = getNcFillValue();
    checkWarnFillValue(res, ncRes);

    // The netcdf fill value takes precendence
    if (ncRes.set_) {
      res = ncRes;
    }
    
    return res;
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while reading a variable's fill value.", ioda_Here()));
  }
}

template <>
VariableCreationParameters Variable_Base<>::getCreationParameters(bool doAtts, bool doDims) const {
  try {
    if (backend_ == nullptr)
      throw Exception("Missing backend or unimplemented backend function.", ioda_Here());
    // If the backend is HH, then it's possible that the hdf5 fill value property is
    // not set (which results in using the netcdf defalut fill value) and we want to check
    // if the netcdf _FillValue variable attribute is being used and if so have that value
    // take precedence. This can be done by calling this object's getFillValue function.
    VariableCreationParameters res = backend_->getCreationParameters(doAtts, doDims);
    res.fillValue_ = getFillValue();
    return res;
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while getting creation-time metadata of a variable.",
      ioda_Here()));
  }
}

template <>
std::vector<Dimensions_t> Variable_Base<>::getChunkSizes() const {
  try {
    if (backend_ == nullptr)
      throw Exception("Missing backend or unimplemented backend function.", ioda_Here());
    return backend_->getChunkSizes();
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while determining a variable's chunking options.",
      ioda_Here()));
  }
}

template <>
std::pair<bool, int> Variable_Base<>::getGZIPCompression() const {
  try {
    if (backend_ == nullptr)
      throw Exception("Missing backend or unimplemented backend function.", ioda_Here());
    return backend_->getGZIPCompression();
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while reading GZIP compression options.", ioda_Here()));
  }
}

template <>
std::tuple<bool, unsigned, unsigned> Variable_Base<>::getSZIPCompression() const {
  try {
    if (backend_ == nullptr)
      throw Exception("Missing backend or unimplemented backend function.", ioda_Here());
    return backend_->getSZIPCompression();
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while reading SZIP compression options.", ioda_Here()));
  }
}

template <>
Dimensions Variable_Base<>::getDimensions() const {
  try {
    if (backend_ == nullptr)
      throw Exception("Missing backend or unimplemented backend function.", ioda_Here());
    return backend_->getDimensions();
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while reading a variable's dimensions.", ioda_Here()));
  }
}

template <>
Variable Variable_Base<>::resize(const std::vector<Dimensions_t>& newDims) {
  try {
    if (backend_ == nullptr)
      throw Exception("Missing backend or unimplemented backend function.", ioda_Here());
    return backend_->resize(newDims);
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while resizing a variable.", ioda_Here()));
  }
}

template <>
Variable Variable_Base<>::attachDimensionScale(unsigned int DimensionNumber,
                                               const Variable& scale) {
  try {
    if (backend_ == nullptr)
      throw Exception("Missing backend or unimplemented backend function.", ioda_Here());
    return backend_->attachDimensionScale(DimensionNumber, scale);
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while attaching a dimension scale to a variable.",
      ioda_Here()));
  }
}

template <>
Variable Variable_Base<>::detachDimensionScale(unsigned int DimensionNumber,
                                               const Variable& scale) {
  try {
    if (backend_ == nullptr)
      throw Exception("Missing backend or unimplemented backend function.", ioda_Here());
    return backend_->detachDimensionScale(DimensionNumber, scale);
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while detaching a dimension "
      "scale from a variable.", ioda_Here()));
  }
}

template <>
Variable Variable_Base<>::setDimScale(const std::vector<Variable>& vdims) {
  try {
    for (unsigned int i = 0; i < vdims.size(); ++i)
      attachDimensionScale(i, vdims[i]);
    return Variable{backend_};
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while setting dimension scales on a variable.",
      ioda_Here()));
  }
}

template <>
Variable Variable_Base<>::setDimScale(const std::vector<Named_Variable>& vdims) {
  try {
    for (unsigned int i = 0; i < vdims.size(); ++i) attachDimensionScale(i, vdims[i].var);
    return Variable{backend_};
  } catch (...) {
    std::throw_with_nested(
      Exception("An exception occurred inside ioda while setting dimension scales on a variable.",
                ioda_Here()));
  }
  
}
template <>
Variable Variable_Base<>::setDimScale(const Variable& dims) {
  return setDimScale(std::vector<Variable>{dims});
}
template <>
Variable Variable_Base<>::setDimScale(const Variable& dim1, const Variable& dim2) {
  return setDimScale(std::vector<Variable>{dim1, dim2});
}
template <>
Variable Variable_Base<>::setDimScale(const Variable& dim1, const Variable& dim2,
                                      const Variable& dim3) {
  return setDimScale(std::vector<Variable>{dim1, dim2, dim3});
}

template <>
bool Variable_Base<>::isDimensionScale() const {
  try {
    if (backend_ == nullptr)
      throw Exception("Missing backend or unimplemented backend function.", ioda_Here());
    return backend_->isDimensionScale();
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while checking if a variable is a dimension scale.",
      ioda_Here()));
  }
}

template <>
Variable Variable_Base<>::setIsDimensionScale(const std::string& dimensionScaleName) {
  try {
    if (backend_ == nullptr)
      throw Exception("Missing backend or unimplemented backend function.", ioda_Here());
    return backend_->setIsDimensionScale(dimensionScaleName);
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while making a variable a dimension scale.",
      ioda_Here()));
  }
}
template <>
Variable Variable_Base<>::getDimensionScaleName(std::string& res) const {
  try {
    if (backend_ == nullptr)
      throw Exception("Missing backend or unimplemented backend function.", ioda_Here());
    return backend_->getDimensionScaleName(res);
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while determining the human-readable "
      "name of a dimension scale.", ioda_Here()));
  }
}
template <>
bool Variable_Base<>::isDimensionScaleAttached(unsigned int DimensionNumber,
                                               const Variable& scale) const {
  try {
    if (backend_ == nullptr)
      throw Exception("Missing backend or unimplemented backend function.", ioda_Here());
    return backend_->isDimensionScaleAttached(DimensionNumber, scale);
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while determining if a dimension scale is "
      "attached to a variable at a specified dimension.", ioda_Here())
      .add("DimensionNumber", DimensionNumber));
  }
}

template <>
std::vector<std::vector<Named_Variable>> Variable_Base<>::getDimensionScaleMappings(
  const std::list<Named_Variable>& scalesToQueryAgainst, bool firstOnly) const {
  try {
    if (backend_ == nullptr)
      throw Exception("Missing backend or unimplemented backend function.", ioda_Here());
    return backend_->getDimensionScaleMappings(scalesToQueryAgainst, firstOnly);
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while determining which scales are attached to "
      "which dimensions of a variable.", ioda_Here()));
  }
}

template <>
Variable Variable_Base<>::write(gsl::span<const char> data, const Type& in_memory_dataType,
                          const Selection& mem_selection, const Selection& file_selection) {
  try {
    if (backend_ == nullptr)
      throw Exception("Missing backend or unimplemented backend function.", ioda_Here());
    return backend_->write(data, in_memory_dataType, mem_selection, file_selection);
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while writing data to a variable.", ioda_Here()));
  }
}

template <>
Variable Variable_Base<>::parallelWrite(gsl::span<const char> data,
                          const Type& in_memory_dataType,
                          const Selection& mem_selection, const Selection& file_selection) {
  try {
    if (backend_ == nullptr)
      throw Exception("Missing backend or unimplemented backend function.", ioda_Here());
    return backend_->parallelWrite(data, in_memory_dataType, mem_selection, file_selection);
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while writing data to a variable.", ioda_Here()));
  }
}

template <>
Variable Variable_Base<>::read(gsl::span<char> data, const Type& in_memory_dataType,
                               const Selection& mem_selection,
                               const Selection& file_selection) const {
  try {
    if (backend_ == nullptr)
      throw Exception("Missing backend or unimplemented backend function.", ioda_Here());
    return backend_->read(data, in_memory_dataType, mem_selection, file_selection);
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while reading data from a variable.", ioda_Here()));
  }
}

template <>
Selections::SelectionBackend_t Variable_Base<>::instantiateSelection(const Selection& sel) const {
  try {
    if (backend_ == nullptr)
      throw Exception("Missing backend or unimplemented backend function.", ioda_Here());
    return backend_->instantiateSelection(sel);
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda.", ioda_Here()));
  }
}

template class Variable_Base<Variable>;  // NOLINT: Bad check result

Variable_Backend::~Variable_Backend() = default;
Variable_Backend::Variable_Backend() : Variable_Base(nullptr) {}

std::vector<std::vector<Named_Variable>> Variable_Backend::getDimensionScaleMappings(
  const std::list<Named_Variable>& scalesToQueryAgainst, bool firstOnly) const {
  try {
    auto dims = this->getDimensions();
    std::vector<std::vector<Named_Variable>> res(gsl::narrow<size_t>(dims.dimensionality));
    for (unsigned i = 0; i < gsl::narrow<unsigned>(dims.dimensionality); ++i) {
      for (const auto& s : scalesToQueryAgainst) {
        if (this->isDimensionScaleAttached(i, s.var)) {
          res[i].push_back(s);
          if (firstOnly) break;
        }
      }
    }

    return res;
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda.", ioda_Here()));
  }
}

VariableCreationParameters Variable_Backend::getCreationParameters(bool doAtts, bool doDims) const {
  try {
    VariableCreationParameters res;

    // Get chunking
    auto chunkinfo = getChunkSizes();
    if (chunkinfo.size()) {
      res.chunk  = true;
      res.chunks = chunkinfo;
    }
    // Get compression
    auto gz = getGZIPCompression();
    if (gz.first) res.compressWithGZIP(gz.second);
    auto sz = getSZIPCompression();
    if (std::get<0>(sz)) res.compressWithSZIP(std::get<1>(sz), std::get<2>(sz));
    // Get fill value
    res.fillValue_ = getFillValue();
    // Attributes (optional)
    if (doAtts) {
      throw Exception("Unimplemented doAtts option.", ioda_Here());
    }
    // Dimensions (optional)
    if (doDims) {
      throw Exception("Unimplemented doDims option.", ioda_Here());
    }

    return res;
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while determining creation-time parameters of a "
      "variable .", ioda_Here()));
  }
}

}  // namespace detail

Variable::~Variable() = default;
Variable::Variable()
    : detail::Variable_Base<Variable>(nullptr),
      _py_isA{this},
      _py_readVector{this},
      _py_readNPArray{this},
      _py_writeVector{this},
      _py_writeNPArray{this},
      _py_scales{this} {}

Variable::Variable(std::shared_ptr<detail::Variable_Backend> b)
    : detail::Variable_Base<Variable>(b),
      _py_isA{this},
      _py_readVector{this},
      _py_readNPArray{this},
      _py_writeVector{this},
      _py_writeNPArray{this},
      _py_scales{this} {}

Variable::Variable(const Variable& r)
    : Variable_Base{r.backend_},
      _py_isA{this},
      _py_readVector{this},
      _py_readNPArray{this},
      _py_writeVector{this},
      _py_writeNPArray{this},
      _py_scales{this} {}

Variable& Variable::operator=(const Variable& r) {
  if (this == &r) return *this;
  backend_   = r.backend_;
  atts       = r.atts;
  _py_scales = detail::python_bindings::VariableScales<Variable>{this};
  _py_isA    = detail::python_bindings::VariableIsA<Variable>{this};

  _py_readVector  = detail::python_bindings::VariableReadVector<Variable>{this};
  _py_readNPArray = detail::python_bindings::VariableReadNPArray<Variable>{this};

  _py_writeVector  = detail::python_bindings::VariableWriteVector<Variable>{this};
  _py_writeNPArray = detail::python_bindings::VariableWriteNPArray<Variable>{this};
  return *this;
}
}  // namespace ioda
