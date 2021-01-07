/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "ioda/Variables/Variable.h"

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
  Expects(backend_ != nullptr);
  return backend_->isA(lhs);
}
template <>
detail::Type_Provider* Variable_Base<>::getTypeProvider() const {
  Expects(backend_ != nullptr);
  return backend_->getTypeProvider();
}

/// \note This is very inefficient. Refactor?
template <>
BasicTypes Variable_Base<>::getBasicType() const {
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
}

template <>
bool Variable_Base<>::hasFillValue() const {
  Expects(backend_ != nullptr);
  return backend_->hasFillValue();
}

template <>
Variable_Base<>::FillValueData_t Variable_Base<>::getFillValue() const {
  Expects(backend_ != nullptr);
  return backend_->getFillValue();
}

template <>
std::vector<Dimensions_t> Variable_Base<>::getChunkSizes() const {
  Expects(backend_ != nullptr);
  return backend_->getChunkSizes();
}

template <>
std::pair<bool, int> Variable_Base<>::getGZIPCompression() const {
  Expects(backend_ != nullptr);
  return backend_->getGZIPCompression();
}

template <>
std::tuple<bool, unsigned, unsigned> Variable_Base<>::getSZIPCompression() const {
  Expects(backend_ != nullptr);
  return backend_->getSZIPCompression();
}

template <>
Dimensions Variable_Base<>::getDimensions() const {
  Expects(backend_ != nullptr);
  return backend_->getDimensions();
}

template <>
Variable Variable_Base<>::resize(const std::vector<Dimensions_t>& newDims) {
  Expects(backend_ != nullptr);
  return backend_->resize(newDims);
}

template <>
Variable Variable_Base<>::attachDimensionScale(unsigned int DimensionNumber, const Variable& scale) {
  Expects(backend_ != nullptr);
  return backend_->attachDimensionScale(DimensionNumber, scale);
}

template <>
Variable Variable_Base<>::detachDimensionScale(unsigned int DimensionNumber, const Variable& scale) {
  Expects(backend_ != nullptr);
  return backend_->detachDimensionScale(DimensionNumber, scale);
}
template <>
Variable Variable_Base<>::setDimScale(const std::vector<Variable>& vdims) {
  for (unsigned int i = 0; i < vdims.size(); ++i) attachDimensionScale(i, vdims[i]);
  return Variable{backend_};
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
Variable Variable_Base<>::setDimScale(const Variable& dim1, const Variable& dim2, const Variable& dim3) {
  return setDimScale(std::vector<Variable>{dim1, dim2, dim3});
}

template <>
bool Variable_Base<>::isDimensionScale() const {
  Expects(backend_ != nullptr);
  return backend_->isDimensionScale();
}

template <>
Variable Variable_Base<>::setIsDimensionScale(const std::string& dimensionScaleName) {
  Expects(backend_ != nullptr);
  return backend_->setIsDimensionScale(dimensionScaleName);
}
template <>
Variable Variable_Base<>::getDimensionScaleName(std::string& res) const {
  Expects(backend_ != nullptr);
  return backend_->getDimensionScaleName(res);
}
template <>
bool Variable_Base<>::isDimensionScaleAttached(unsigned int DimensionNumber, const Variable& scale) const {
  Expects(backend_ != nullptr);
  return backend_->isDimensionScaleAttached(DimensionNumber, scale);
}

template <>
std::vector<std::vector<std::pair<std::string, Variable>>> Variable_Base<>::getDimensionScaleMappings(
  const std::list<std::pair<std::string, Variable>>& scalesToQueryAgainst, bool firstOnly) const {
  Expects(backend_ != nullptr);
  return backend_->getDimensionScaleMappings(scalesToQueryAgainst, firstOnly);
}

template <>
Variable Variable_Base<>::write(gsl::span<char> data, const Type& in_memory_dataType,
                                const Selection& mem_selection, const Selection& file_selection) {
  Expects(backend_ != nullptr);
  return backend_->write(data, in_memory_dataType, mem_selection, file_selection);
}

template <>
Variable Variable_Base<>::read(gsl::span<char> data, const Type& in_memory_dataType,
                               const Selection& mem_selection, const Selection& file_selection) const {
  Expects(backend_ != nullptr);
  return backend_->read(data, in_memory_dataType, mem_selection, file_selection);
}

template class Variable_Base<Variable>;  // NOLINT: Bad check result

Variable_Backend::~Variable_Backend() = default;
Variable_Backend::Variable_Backend() : Variable_Base(nullptr) {}

std::vector<std::vector<std::pair<std::string, Variable>>> Variable_Backend::getDimensionScaleMappings(
  const std::list<std::pair<std::string, Variable>>& scalesToQueryAgainst, bool firstOnly) const {
  auto dims = this->getDimensions();
  std::vector<std::vector<std::pair<std::string, Variable>>> res(gsl::narrow<size_t>(dims.dimensionality));
  for (unsigned i = 0; i < gsl::narrow<unsigned>(dims.dimensionality); ++i) {
    for (const auto& s : scalesToQueryAgainst) {
      if (this->isDimensionScaleAttached(i, s.second)) {
        res[i].push_back(s);
        if (firstOnly) break;
      }
    }
  }

  return res;
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
  backend_ = r.backend_;
  atts = r.atts;
  _py_scales = detail::python_bindings::VariableScales<Variable>{this};
  _py_isA = detail::python_bindings::VariableIsA<Variable>{this};

  _py_readVector = detail::python_bindings::VariableReadVector<Variable>{this};
  _py_readNPArray = detail::python_bindings::VariableReadNPArray<Variable>{this};

  _py_writeVector = detail::python_bindings::VariableWriteVector<Variable>{this};
  _py_writeNPArray = detail::python_bindings::VariableWriteNPArray<Variable>{this};
  return *this;
}
}  // namespace ioda
