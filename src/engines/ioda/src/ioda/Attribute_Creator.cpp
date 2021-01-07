/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "ioda/Attributes/Attribute_Creator.h"

namespace ioda {
namespace detail {
Attribute_Creator_Base::~Attribute_Creator_Base() = default;

Attribute_Creator_Base::Attribute_Creator_Base(const std::string& name) : name_(name) {}

}  // namespace detail

Attribute_Creator_Store::Attribute_Creator_Store() = default;
Attribute_Creator_Store::~Attribute_Creator_Store() = default;

void Attribute_Creator_Store::apply(Has_Attributes& obj) const {
  for (const auto& a : atts_) a->apply(obj);
}

}  // namespace ioda
