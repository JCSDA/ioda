/*
 * (C) Copyright 2017-2020 Ryan Honeyager (ryan@honeyager.info)
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "./HH/HH-Filters.h"

#include <hdf5.h>
#include <hdf5_hl.h>

#include <algorithm>
#include <numeric>
#include <set>

#include "./HH/HH-types.h"
#include "./HH/HH-variables.h"
#include "./HH/Handles.h"

namespace ioda {
namespace detail {
namespace Engines {
namespace HH {
std::pair<bool, bool> isFilteravailable(H5Z_filter_t filt) {
  unsigned int filter_config = 0;
  htri_t avl                 = H5Zfilter_avail(filt);
  if (avl <= 0) return std::make_pair(false, false);
  herr_t err = H5Zget_filter_info(filt, &filter_config);
  if (err < 0) throw;
  bool compress   = false;
  bool decompress = false;
  if (filter_config & H5Z_FILTER_CONFIG_ENCODE_ENABLED)
    compress = true;  // NOLINT(hicpp-signed-bitwise): Check is a false positive.
  if (filter_config & H5Z_FILTER_CONFIG_DECODE_ENABLED)
    decompress = true;  // NOLINT(hicpp-signed-bitwise): Check is a false positive.
  return std::make_pair(compress, decompress);
}

bool CanUseSZIP(HH_hid_t dtype) {
  if (!isFilteravailable(H5Z_FILTER_SZIP).first) return false;

  H5T_class_t c = H5Tget_class(dtype());  // NOLINT: Enumerator scoping
  switch (c) {
    case H5T_ARRAY:
    case H5T_VLEN:
    case H5T_ENUM:
    case H5T_COMPOUND:
    case H5T_REFERENCE:
      return false;
      break;
    default:
      break;
  }

  return true;
}

Filters::Filters(HH_hid_t newbase) : pl(newbase) {}
Filters::~Filters() = default;

std::vector<Filters::filter_info> Filters::get() const {
  int nfilts = H5Pget_nfilters(pl());
  if (nfilts < 0) throw;
  std::vector<filter_info> res;
  for (int i = 0; i < nfilts; ++i) {
    filter_info obj;
    size_t cd_nelems = 0;
    obj.id = H5Pget_filter2(pl(), i, &(obj.flags), &cd_nelems, nullptr, 0, nullptr, nullptr);
    obj.cd_values.resize(cd_nelems);
    H5Pget_filter2(pl(), i, &(obj.flags), &cd_nelems, obj.cd_values.data(), 0, nullptr, nullptr);

    res.push_back(obj);
  }
  return res;
}

void Filters::append(const std::vector<filter_info>& filters) {
  for (const auto& f : filters) {
    herr_t res = H5Pset_filter(pl(), f.id, f.flags, f.cd_values.size(), f.cd_values.data());
    if (res < 0) throw;
  }
}

void Filters::set(const std::vector<filter_info>& filters) {
  if (H5Premove_filter(pl(), H5Z_FILTER_ALL) < 0) throw;
  append(filters);
}

void Filters::clear() {
  if (H5Premove_filter(pl(), H5Z_FILTER_ALL) < 0) throw;
}

bool Filters::has(H5Z_filter_t id) const {
  auto fi = get();
  auto res
    = std::find_if(fi.cbegin(), fi.cend(), [&id](const filter_info& f) { return f.id == id; });
  return (res != fi.cend());
}

Filters::FILTER_T Filters::getType(const filter_info& it) {
  if ((it.id == H5Z_FILTER_SHUFFLE)) return FILTER_T::SHUFFLE;
  if ((it.id == H5Z_FILTER_DEFLATE)) return FILTER_T::COMPRESSION;
  if ((it.id == H5Z_FILTER_SZIP)) return FILTER_T::COMPRESSION;
  if ((it.id == H5Z_FILTER_NBIT)) return FILTER_T::COMPRESSION;
  if ((it.id == H5Z_FILTER_SCALEOFFSET)) return FILTER_T::COMPRESSION;
  return FILTER_T::OTHER;
}

bool Filters::isA(const filter_info& it, Filters::FILTER_T typ) {
  auto ft = getType(it);
  return (ft == typ);
}

void Filters::appendOfType(const std::vector<filter_info>& filters, FILTER_T typ) {
  for (auto it = filters.cbegin(); it != filters.cend(); ++it) {
    if (isA(*it, typ)) {
      herr_t res
        = H5Pset_filter(pl(), it->id, it->flags, it->cd_values.size(), it->cd_values.data());
      if (res < 0) throw;
    }
  }
}

void Filters::removeOfType(FILTER_T typ) {
  auto fils = get();
  clear();
  for (auto it = fils.cbegin(); it != fils.cend(); ++it) {
    if (!isA(*it, typ)) {
      herr_t res
        = H5Pset_filter(pl(), it->id, it->flags, it->cd_values.size(), it->cd_values.data());
      if (res < 0) throw;
    }
  }
}

void Filters::setShuffle() {
  if (has(H5Z_FILTER_SHUFFLE)) return;
  auto fils = get();
  clear();
  appendOfType(fils, FILTER_T::SCALE);
  if (0 > H5Pset_shuffle(pl())) throw;
  appendOfType(fils, FILTER_T::COMPRESSION);
  appendOfType(fils, FILTER_T::OTHER);
}

void Filters::setSZIP(unsigned int optm, unsigned int ppb) {
  if (has(H5Z_FILTER_SZIP)) return;
  auto fils = get();
  clear();
  appendOfType(fils, FILTER_T::SCALE);
  appendOfType(fils, FILTER_T::SHUFFLE);

  // unsigned int optm = H5_SZIP_EC_OPTION_MASK;
  // unsigned int ppb = 16;
  // if (pixels_per_block.has_value()) ppb = pixels_per_block.value();

  if (0 > H5Pset_szip(pl(), optm, ppb)) throw;
  appendOfType(fils, FILTER_T::OTHER);
}

void Filters::setGZIP(unsigned int level) {
  auto fils = get();
  clear();
  appendOfType(fils, FILTER_T::SCALE);
  appendOfType(fils, FILTER_T::SHUFFLE);
  if (0 > H5Pset_deflate(pl(), level)) throw;
  appendOfType(fils, FILTER_T::OTHER);
}

void Filters::setScaleOffset(H5Z_SO_scale_type_t scale_type, int scale_factor) {
  auto fils = get();
  clear();
  if (0 > H5Pset_scaleoffset(pl(), scale_type, scale_factor)) throw;
  appendOfType(fils, FILTER_T::SHUFFLE);
  appendOfType(fils, FILTER_T::COMPRESSION);
  appendOfType(fils, FILTER_T::OTHER);
}
}  // namespace HH
}  // namespace Engines
}  // namespace detail
}  // namespace ioda
