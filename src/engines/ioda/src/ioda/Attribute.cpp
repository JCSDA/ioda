/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "ioda/Attributes/Attribute.h"

namespace ioda {

namespace detail {

template <>
Attribute_Base<>::~Attribute_Base() = default;
template <>
Attribute_Base<>::Attribute_Base(std::shared_ptr<Attribute_Backend> hnd_attr)
    : backend_(hnd_attr) {}

// Attribute Attribute::writeFixedLengthString(const std::string& data) { Expects(backend_ !=
// nullptr && "Unimplemented function for backend");  return backend_->writeFixedLengthString(data);
// } template<> Type Attribute_Base<>::getType() const { Expects(backend_ != nullptr); return
// backend_->getType(); }

template <>
Dimensions Attribute_Base<>::getDimensions() const {
  Expects(backend_ != nullptr);
  return backend_->getDimensions();
}

template <>
bool Attribute_Base<>::isA(Type lhs) const {
  Expects(backend_ != nullptr);
  return backend_->isA(lhs);
}

template <>
detail::Type_Provider* Attribute_Base<>::getTypeProvider() const {
  Expects(backend_ != nullptr);
  return backend_->getTypeProvider();
}

template <>
Attribute Attribute_Base<>::write(gsl::span<char> data, const Type& in_memory_dataType) {
  Expects(backend_ != nullptr);
  return backend_->write(data, in_memory_dataType);
}

template <>
Attribute Attribute_Base<>::read(gsl::span<char> data, const Type& in_memory_dataType) const {
  Expects(backend_ != nullptr);
  // Expects(data.size_bytes() >= getStorageSize()); // Not always applicable for strings.
  return backend_->read(data, in_memory_dataType);
}

template class Attribute_Base<Attribute>;  // NOLINT: Bad check result

Attribute_Backend::~Attribute_Backend() = default;
Attribute_Backend::Attribute_Backend() : Attribute_Base{nullptr} {}
}  // namespace detail

Attribute::~Attribute() = default;
Attribute::Attribute()
    : Attribute_Base(nullptr),
      _py_isA{this},
      _py_readSingle{this},
      _py_readVector{this},
      _py_readNPArray{this},
      _py_writeSingle{this},
      _py_writeVector{this},
      _py_writeNPArray{this} {}
Attribute::Attribute(std::shared_ptr<detail::Attribute_Backend> b)
    : Attribute_Base{b},
      _py_isA{this},
      _py_readSingle{this},
      _py_readVector{this},
      _py_readNPArray{this},
      _py_writeSingle{this},
      _py_writeVector{this},
      _py_writeNPArray{this} {}
Attribute::Attribute(const Attribute& r)
    : Attribute_Base{r.backend_},
      _py_isA{this},
      _py_readSingle{this},
      _py_readVector{this},
      _py_readNPArray{this},
      _py_writeSingle{this},
      _py_writeVector{this},
      _py_writeNPArray{this} {}
Attribute& Attribute::operator=(const Attribute& r) {
  if (this == &r) return *this;
  backend_        = r.backend_;
  _py_isA         = detail::python_bindings::AttributeIsA<Attribute>{this};
  _py_readSingle  = detail::python_bindings::AttributeReadSingle<Attribute>{this};
  _py_readVector  = detail::python_bindings::AttributeReadVector<Attribute>{this};
  _py_readNPArray = detail::python_bindings::AttributeReadNPArray<Attribute>{this};

  _py_writeSingle  = detail::python_bindings::AttributeWriteSingle<Attribute>{this};
  _py_writeVector  = detail::python_bindings::AttributeWriteVector<Attribute>{this};
  _py_writeNPArray = detail::python_bindings::AttributeWriteNPArray<Attribute>{this};
  return *this;
}
}  // namespace ioda
