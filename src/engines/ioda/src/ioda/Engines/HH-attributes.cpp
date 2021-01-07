/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "ioda/Engines/HH/HH-attributes.h"

#include "ioda/Engines/HH/HH-types.h"
#include "ioda/Misc/Dimensions.h"

namespace ioda {
namespace detail {
namespace Engines {
namespace HH {
HH_Attribute_Backend::HH_Attribute_Backend() : backend_(::HH::Types::HH_hid_t::dummy()) {}
HH_Attribute_Backend::HH_Attribute_Backend(::HH::Attribute h) : backend_(h) {}
HH_Attribute_Backend::~HH_Attribute_Backend() = default;

detail::Type_Provider* HH_Attribute_Backend::getTypeProvider() const { return HH_Type_Provider::instance(); }

Attribute HH_Attribute_Backend::write(gsl::span<char> data, const Type& in_memory_dataType) {
  try {
    auto typeBackend = std::dynamic_pointer_cast<HH_Type>(in_memory_dataType.getBackend());
    backend_.writeDirect(data, typeBackend->handle);
    return Attribute{shared_from_this()};
  } catch (std::bad_cast&) {
    throw;
  }
}

// Attribute Attribute_Backend::writeFixedLengthString(const std::string& data) {
//	Expects(backend_ != nullptr);
//	backend_->writeFixedLengthString(data);
//	return *this;
//}

Attribute HH_Attribute_Backend::read(gsl::span<char> data, const Type& in_memory_dataType) const {
  try {
    auto typeBackend = std::dynamic_pointer_cast<HH_Type>(in_memory_dataType.getBackend());
    backend_.readDirect(data, typeBackend->handle);

    // Override for old-format ioda files:
    // Unfortunately, v0 ioda files have an odd mixture
    // of ascii vs unicode strings, as well as
    // fixed and variable-length strings.
    // We try and fix a few of these issues here.

    // Not the usual return Attribute{ shared_from_this() } because
    // of const-ness. I don't break that guarantee here, but it's a
    // small limitation on the interface.
    return Attribute{std::make_shared<HH_Attribute_Backend>(*this)};
  } catch (std::bad_cast&) {
    throw;
  }
}

bool HH_Attribute_Backend::isA(Type lhs) const {
  try {
    auto typeBackend = std::dynamic_pointer_cast<HH_Type>(lhs.getBackend());

    // Override for old-format ioda files:
    // Unfortunately, v0 ioda files have an odd mixture
    // of ascii vs unicode strings, as well as
    // fixed and variable-length strings.
    // We try and fix a few of these issues here.

    // Do we expect a string of any type?
    // Is the object a string of any type?
    // If both are true, then we just return true.
    H5T_class_t cls_lhs = H5Tget_class(typeBackend->handle.get());
    H5T_class_t cls_my = H5Tget_class(backend_.getType().get());
    if (cls_lhs == H5T_STRING && cls_my == H5T_STRING) return true;

    return backend_.isOfType(typeBackend->handle);
  } catch (std::bad_cast&) {
    throw;
  }
}

/*
Type HH_Attribute_Backend::getType() const {
        auto htype = backend_.getType().get();
        return mapType(htype);
}
*/

// bool Attribute_Backend::isOfType(Encapsulated_Handle lhs, Encapsulated_Handle rhs) const;
// Encapsulated_Handle Attribute_Backend::getSpace() const;

Dimensions HH_Attribute_Backend::getDimensions() const {
  auto back_dims = backend_.getDimensions();
  Dimensions ret;
  ret.numElements = gsl::narrow<decltype(Dimensions::numElements)>(back_dims.numElements);
  ret.dimensionality = gsl::narrow<decltype(Dimensions::dimensionality)>(back_dims.dimensionality);
  for (const auto& d : back_dims.dimsCur) ret.dimsCur.push_back(gsl::narrow<Dimensions_t>(d));
  for (const auto& d : back_dims.dimsMax) ret.dimsMax.push_back(gsl::narrow<Dimensions_t>(d));

  return ret;
}

HH_HasAttributes_Backend::HH_HasAttributes_Backend() : backend_(::HH::Handles::HH_hid_t::dummy()) {}
HH_HasAttributes_Backend::HH_HasAttributes_Backend(::HH::Has_Attributes b) : backend_(b) {}
HH_HasAttributes_Backend::~HH_HasAttributes_Backend() = default;
detail::Type_Provider* HH_HasAttributes_Backend::getTypeProvider() const {
  return HH_Type_Provider::instance();
}

std::vector<std::string> HH_HasAttributes_Backend::list() const { return backend_.list(); }
bool HH_HasAttributes_Backend::exists(const std::string& attname) const { return backend_.exists(attname); }
void HH_HasAttributes_Backend::remove(const std::string& attname) { return backend_.remove(attname); }
Attribute HH_HasAttributes_Backend::open(const std::string& name) const {
  auto res = backend_.open(name);
  auto b = std::make_shared<HH_Attribute_Backend>(res);
  Attribute att{b};
  return att;
  // return Attribute{ Attribute_Backend{backend_.open(name)} };
}
Attribute HH_HasAttributes_Backend::create(const std::string& attrname, const Type& in_memory_dataType,
                                           const std::vector<Dimensions_t>& dimensions) {
  try {
    auto typeBackend = std::dynamic_pointer_cast<HH_Type>(in_memory_dataType.getBackend());
    std::vector<hsize_t> hDims;
    hDims.reserve(dimensions.size());
    for (const auto& d : dimensions) hDims.push_back(gsl::narrow<hsize_t>(d));
    // Note: Char parameter added for HH compatability. Completely unnecessary.
    // Should update HH to remove the need for this.

    // Explicitly and slowly stating this for debugging.
    auto res = backend_.create<char>(attrname, hDims, typeBackend->handle);
    auto b = std::make_shared<HH_Attribute_Backend>(res);
    Attribute att{b};
    return att;
  } catch (std::bad_cast&) {
    throw;
  }
}
void HH_HasAttributes_Backend::rename(const std::string& oldName, const std::string& newName) {
  backend_.rename(oldName, newName);
}

}  // namespace HH
}  // namespace Engines
}  // namespace detail
}  // namespace ioda
