#pragma once
/*
 * (C) Copyright 2017-2020 Ryan Honeyager (ryan@honeyager.info)
 *
 * This software is licensed under the terms of the BSD 2-Clause License.
 * For details, see the LICENSE file.
 */
#include <hdf5.h>

#include <algorithm>
#include <gsl/gsl-lite.hpp>
#include <optional>
#include <tuple>
#include <utility>
#include <vector>

#include "Handles.hpp"
#include "Handles_HDF.hpp"
#include "Tags.hpp"

namespace HH {
namespace PL {
enum class CompressionType { NONE, ANY, GZIP, SZIP };
}
namespace Tags {
namespace PropertyLists {
namespace _detail {
struct tag_LinkCreationPlist {};
struct tag_DatasetCreationPlist {};
struct tag_DatasetAccessPlist {};
struct tag_xferPlist {};

struct tag_chunking {};
struct tag_do_shuffle {};
struct tag_compression_type {};
struct tag_gzip_level {};
struct tag_szip_level {};
struct tag_szip_options {};
template <class T>
struct tag_fill_value {};

struct tag_filecacheparams {};
struct tag_filecacheparams_data {
  size_t rdcc_nslots = 521;
  size_t rdcc_nbytes = 1 * 1024 * 1024;
  double rdcc_w0 = 0.75;

  tag_filecacheparams_data(size_t rdcc_nslots, size_t rdcc_nbytes, double rdcc_w0)
      : rdcc_nslots(rdcc_nslots), rdcc_nbytes(rdcc_nbytes), rdcc_w0(rdcc_w0) {}
  tag_filecacheparams_data() {}
};
struct tag_create_intermediate_group {};
}  // namespace _detail
typedef Tag<_detail::tag_LinkCreationPlist, HH_hid_t> t_LinkCreationPlist;
typedef Tag<_detail::tag_DatasetCreationPlist, HH_hid_t> t_DatasetCreationPlist;
typedef Tag<_detail::tag_DatasetAccessPlist, HH_hid_t> t_DatasetAccessPlist;
typedef Tag<_detail::tag_xferPlist, HH_hid_t> t_xferPlist;

typedef Tag<_detail::tag_create_intermediate_group, bool> t_create_intermediate_group;
typedef Tag<_detail::tag_chunking, std::initializer_list<hsize_t> > t_Chunking;
typedef Tag<_detail::tag_szip_options, bool> t_DoShuffle;
typedef Tag<_detail::tag_compression_type, PL::CompressionType> t_CompressionType;
typedef Tag<_detail::tag_gzip_level, int> t_GZIPlevel;
typedef Tag<_detail::tag_szip_level, unsigned int> t_SZIP_PixelsPerBlock;
typedef Tag<_detail::tag_szip_options, unsigned int> t_SZIPopts;
// template <class T> struct tag_fill_value {};
template <class T>
using t_FillValue = Tag<_detail::tag_fill_value<T>, T>;

typedef Tag<_detail::tag_filecacheparams, _detail::tag_filecacheparams_data> t_FileCacheParams;
}  // namespace PropertyLists
}  // namespace Tags
namespace PL {
inline HH_hid_t copy_plist(HH_hid_t pl) {
  hid_t plid = H5Pcopy(pl());
  Expects(plid >= 0);
  HH_hid_t plb(plid, Handles::Closers::CloseHDF5PropertyList::CloseP);
  return plb;
}

inline std::pair<bool, bool> isFilteravailable(H5Z_filter_t filt) {
  unsigned int filter_config = 0;
  htri_t avl = H5Zfilter_avail(filt);
  if (avl <= 0) return std::make_pair(false, false);
  herr_t err = H5Zget_filter_info(filt, &filter_config);
  Expects(err >= 0);
  bool compress = false;
  bool decompress = false;
  if (filter_config & H5Z_FILTER_CONFIG_ENCODE_ENABLED) compress = true;
  if (filter_config & H5Z_FILTER_CONFIG_DECODE_ENABLED) decompress = true;
  return std::make_pair(compress, decompress);
}

template <class DataType>
bool CanUseSZIP(HH_hid_t dtype = HH::Types::GetHDF5Type<DataType>()) {
  // Check restrictions on SZIP first.
  // SZIP cannot be applied to compound, array, variable-length,
  // enumerative or user-defined datatypes.

  if (!isFilteravailable(H5Z_FILTER_SZIP).first) return false;

  H5T_class_t c = H5Tget_class(dtype());
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

struct DatasetCreationPListProperties {
  bool shuffle = false;
  bool compress = false;
  bool gzip = false;
  bool szip = false;
  int gzip_level = 6;
  unsigned int szip_PixelsPerBlock = 16;
  unsigned int szip_options = H5_SZIP_EC_OPTION_MASK;
};

struct PL {
private:
  HH_hid_t base;

public:
  explicit PL(HH_hid_t newbase) : base(newbase), filters(newbase) {}
  static PL create(hid_t typ) {
    hid_t plid = H5Pcreate(typ);
    Expects(plid >= 0);
    HH_hid_t pl(plid, Handles::Closers::CloseHDF5PropertyList::CloseP);
    return PL(pl);
  }
  static PL createDatasetCreation() {
    hid_t plid = H5Pcreate(H5P_DATASET_CREATE);
    Expects(plid >= 0);
    HH_hid_t pl(plid, Handles::Closers::CloseHDF5PropertyList::CloseP);
    return PL(pl);
  }
  static PL createFileAccess() {
    hid_t plid = H5Pcreate(H5P_FILE_ACCESS);
    Expects(plid >= 0);
    HH_hid_t pl(plid, Handles::Closers::CloseHDF5PropertyList::CloseP);
    return PL(pl);
  }
  static PL createLinkCreation() {
    hid_t plid = H5Pcreate(H5P_LINK_CREATE);
    Expects(plid >= 0);
    HH_hid_t pl(plid, Handles::Closers::CloseHDF5PropertyList::CloseP);
    return PL(pl);
  }
  virtual ~PL() {}

  HH_hid_t get() const { return base; }
  hid_t operator()() const { return base(); }

  PL clone() const {
    hid_t plid = H5Pcopy(base());
    Expects(plid >= 0);
    HH_hid_t pl(plid, Handles::Closers::CloseHDF5PropertyList::CloseP);
    return PL(pl);
  }

  /** \brief Order-obeying filter insertions and replacements

  Filters will be repeatedly removed and reinserted to get the desired filter order.
  The desired filter order is:
  - Shuffling
  - Compression

  Life would be easier if HDF5 allowed for an easy way to insert filters at specified orderings.
  **/
  struct Filters {
  private:
    HH_hid_t pl;

  public:
    explicit Filters(HH_hid_t newbase) : pl(newbase) {}
    virtual ~Filters() {}
    /// \see https://support.hdfgroup.org/HDF5/doc/RM/RM_H5P.html#Property-SetFilter for meanings
    struct filter_info {
      H5Z_filter_t id;
      unsigned int flags;
      std::vector<unsigned int> cd_values;
    };
    /// Get a vector of the filters that are implemented
    std::vector<filter_info> get() const {
      int nfilts = H5Pget_nfilters(pl());
      Expects(nfilts >= 0);
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
    /// Append the filters to a property list.
    void append(const std::vector<filter_info>& filters) {
      for (const auto& f : filters) {
        herr_t res = H5Pset_filter(pl(), f.id, f.flags, f.cd_values.size(), f.cd_values.data());
        Expects(res >= 0);
      }
    }
    /// Set the filters to a property list. Clears existing filters.
    void set(const std::vector<filter_info>& filters) {
      H5Premove_filter(pl(), H5Z_FILTER_ALL);
      append(filters);
    }
    void clear() { H5Premove_filter(pl(), H5Z_FILTER_ALL); }
    std::optional<filter_info> has(H5Z_filter_t id) const {
      auto fi = get();
      auto res = std::find_if(fi.cbegin(), fi.cend(), [&id](const filter_info& f) { return f.id == id; });
      if (res != fi.cend()) return *res;
      return {};
    }

    enum class FILTER_T { SHUFFLE, COMPRESSION, OTHER };

    static FILTER_T getType(const filter_info& it) {
      if ((it.id == H5Z_FILTER_SHUFFLE)) return FILTER_T::SHUFFLE;
      if ((it.id == H5Z_FILTER_DEFLATE)) return FILTER_T::COMPRESSION;
      if ((it.id == H5Z_FILTER_SZIP)) return FILTER_T::COMPRESSION;
      if ((it.id == H5Z_FILTER_NBIT)) return FILTER_T::COMPRESSION;
      if ((it.id == H5Z_FILTER_SCALEOFFSET)) return FILTER_T::COMPRESSION;
      return FILTER_T::OTHER;
    }
    static bool isA(const filter_info& it, FILTER_T typ) {
      auto ft = getType(it);
      if (ft == typ) return true;
      return false;
    }
    void appendOfType(const std::vector<filter_info>& filters, FILTER_T typ) {
      for (auto it = filters.cbegin(); it != filters.cend(); ++it) {
        if (isA(*it, typ)) {
          herr_t res = H5Pset_filter(pl(), it->id, it->flags, it->cd_values.size(), it->cd_values.data());
          Expects(res >= 0);
        }
      }
    }
    void removeOfType(FILTER_T typ) {
      auto fils = get();
      clear();
      for (auto it = fils.cbegin(); it != fils.cend(); ++it) {
        if (!isA(*it, typ)) {
          herr_t res = H5Pset_filter(pl(), it->id, it->flags, it->cd_values.size(), it->cd_values.data());
          Expects(res >= 0);
        }
      }
    }

    void setShuffle() {
      if (has(H5Z_FILTER_SHUFFLE).has_value()) return;
      auto fils = get();
      clear();
      Expects(0 <= H5Pset_shuffle(pl()));  // Bit shuffling.
      appendOfType(fils, FILTER_T::COMPRESSION);
      appendOfType(fils, FILTER_T::OTHER);
    }
    void setSZIP(std::optional<unsigned int> options_mask, std::optional<unsigned int> pixels_per_block) {
      if (has(H5Z_FILTER_SZIP).has_value()) return;
      auto fils = get();
      clear();
      appendOfType(fils, FILTER_T::SHUFFLE);

      unsigned int optm = H5_SZIP_EC_OPTION_MASK;
      if (options_mask.has_value()) optm = options_mask.value();
      unsigned int ppb = 16;
      if (pixels_per_block.has_value()) ppb = pixels_per_block.value();

      Expects(0 <= H5Pset_szip(pl(), optm, ppb));
      appendOfType(fils, FILTER_T::OTHER);
    }
    void setGZIP(std::optional<unsigned int> level) {
      if (has(H5Z_FILTER_DEFLATE).has_value()) return;
      auto fils = get();
      clear();
      appendOfType(fils, FILTER_T::SHUFFLE);
      unsigned int lv = 6;
      if (level.has_value()) lv = level.value();
      Expects(0 <= H5Pset_deflate(pl(), lv));
      appendOfType(fils, FILTER_T::OTHER);
    }

  } filters;

  /**
  \brief Dataset creation plist tagged function

  - Takes a property list and applies various operations on it.
  - Sets chunking, compression, shuffling, fill value, and various pertinent sub-options.
  - Will read and re-order the existing property list to attain the end user's goals.
  - Ideal filter ordering:
  - Shuffling
  - Compression (SZIP preferred, then GZIP)
  - Shuffling can be explicitly turned off or on. If not specified, then it is turned off or
  on when compression is enabled or disabled.
  - Compression is a bit odd. Can be NONE, ANY, GZIP or SZIP.
  - If NONE, remove any compression options. Does not touch shuffling unless explicitly told.
  - If explicit SZIP, (shuffle) and apply SZIP.
  If existing SZIP, then preserve its options, unless these are overwritten.
  If no existing SZIP, then take either specified or default options.
  - If explicit GZIP, (shuffle) and apply GZIP.
  If existing GZIP, then preserve its options, unless these are overwritten.
  If no existing GZIP, then take either specified or default options.
  - If ANY, then pick first of (SZIP, GZIP, NONE).
  Query for the availability of the different filters when making the selection.
  Overrides an existing choice, as ANY had to be user-specified.
  **/
  template <class DataType, class... Args>
  PL setDatasetCreationPList(std::tuple<Args...> vals) {
    using namespace Tags;
    using namespace Tags::PropertyLists;
    typedef std::tuple<Args...> vals_t;

    auto dtype = t_datatype(HH::Types::GetHDF5Type<DataType>());
    getOptionalValue(dtype, vals);
    constexpr bool hasManualChunking = HH::Tags::has_type<t_Chunking, vals_t>::value;
    // Ignoring these for now. Chunking should be manually specified.
    // constexpr bool hasDimensions = HH::Tags::has_type<t_dimensions, vals_t >::value;
    // constexpr bool canChunk = (hasManualChunking || hasDimensions);

    constexpr bool hasDoShuffle = HH::Tags::has_type<t_DoShuffle, vals_t>::value;
    constexpr bool hasCompression = HH::Tags::has_type<t_CompressionType, vals_t>::value;
    constexpr bool hasGZlevel = HH::Tags::has_type<t_GZIPlevel, vals_t>::value;
    constexpr bool hasSZIPppb = HH::Tags::has_type<t_SZIP_PixelsPerBlock, vals_t>::value;
    constexpr bool hasSZIPopts = HH::Tags::has_type<t_SZIPopts, vals_t>::value;

    t_DoShuffle doShuffle(false);
    getOptionalValue(doShuffle, vals);
    t_CompressionType compressionType(CompressionType::NONE);
    getOptionalValue(compressionType, vals);
    t_GZIPlevel gz_level(6);
    getOptionalValue(gz_level, vals);
    t_SZIP_PixelsPerBlock sz_ppb(16);
    getOptionalValue(sz_ppb, vals);
    t_SZIPopts sz_opts(H5_SZIP_EC_OPTION_MASK);
    getOptionalValue(sz_opts, vals);
    constexpr bool hasFillValue = HH::Tags::has_type<t_FillValue<DataType>, vals_t>::value;
    // t_FillValue<DataType> fill; // Used only if needed later

    // Shuffing
    if (hasDoShuffle) {
      filters.removeOfType(PL::Filters::FILTER_T::SHUFFLE);
      if (doShuffle.data) filters.setShuffle();
    } else if (hasCompression) {
      if (compressionType.data == CompressionType::NONE) {
        filters.removeOfType(PL::Filters::FILTER_T::SHUFFLE);
      } else {
        filters.removeOfType(PL::Filters::FILTER_T::SHUFFLE);
        filters.setShuffle();
      }
    }

    // Compression
    if (hasCompression) {
      auto addSZIP = [&]() {
        std::optional<unsigned int> opts = {}, ppb = {};
        if (hasSZIPppb) ppb = sz_ppb.data;
        if (hasSZIPopts) opts = sz_opts.data;
        filters.setSZIP(opts, ppb);
      };
      auto addGZIP = [&]() {
        std::optional<unsigned int> clev = {};
        if (hasGZlevel) clev = gz_level.data;
        filters.setGZIP(clev);
      };
      if (compressionType.data == CompressionType::NONE)
        filters.removeOfType(PL::Filters::FILTER_T::COMPRESSION);
      // NOTE: Shuffling would have been turned on in the above code block.
      else if (compressionType.data == CompressionType::SZIP)
        addSZIP();
      else if (compressionType.data == CompressionType::GZIP)
        addGZIP();
      else if (compressionType.data == CompressionType::ANY) {
        // SZIP checks filter availability and suitability for this data type
        if (CanUseSZIP<DataType>(dtype.data)) addSZIP();
        // GZIP just checks filter availability.
        else if (isFilteravailable(H5Z_FILTER_DEFLATE).first == true)
          addGZIP();
      }
    }

    // Chunking
    if (hasManualChunking) {
      t_Chunking manualChunking;
      getOptionalValue(manualChunking, vals);
      Expects(
        0 <= H5Pset_chunk(base(), static_cast<int>(manualChunking.data.size()), manualChunking.data.begin()));
    }

    // Fill value
    if (hasFillValue) {
      t_FillValue<DataType> fill;
      getOptionalValue(fill, vals);
      Expects(0 <= H5Pset_fill_value(base(), dtype.data(), &(fill.data)));
    }

    return *this;
  }
  template <class DataType, class... Args>
  PL setDatasetCreationPList(Args... args) {
    auto t = std::make_tuple(args...);
    return setDatasetCreationPList<DataType, Args...>(t);
  }

  template <class... Args>
  PL setFileAccessPList(std::tuple<Args...> vals) {
    using namespace Tags;
    using namespace Tags::PropertyLists;
    typedef std::tuple<Args...> vals_t;

    constexpr bool hasSetCache = Tags::has_type<t_FileCacheParams, vals_t>::value;
    if (hasSetCache) {
      t_FileCacheParams cps;
      getOptionalValue(cps, vals);
      Expects(0 <= H5Pset_cache(base(), 0, cps.data.rdcc_nslots, cps.data.rdcc_nbytes, cps.data.rdcc_w0));
    }

    return *this;
  }
  template <class... Args>
  PL setFileAccessPList(Args... args) {
    auto t = std::make_tuple(args...);
    return setFileAccessPList<Args...>(t);
  }

  template <class... Args>
  PL setLinkCreationPList(std::tuple<Args...> vals) {
    using namespace Tags;
    using namespace Tags::PropertyLists;
    typedef std::tuple<Args...> vals_t;

    constexpr bool hasCreateIntGrp = Tags::has_type<t_create_intermediate_group, vals_t>::value;
    if (hasCreateIntGrp) {
      t_create_intermediate_group cps;
      getOptionalValue(cps, vals);
      if (cps.data == true)
        Expects(0 <= H5Pset_create_intermediate_group(base(), 1));
      else
        Expects(0 <= H5Pset_create_intermediate_group(base(), -1));
    }

    return *this;
  }
  template <class... Args>
  PL setLinkCreationPList(Args... args) {
    auto t = std::make_tuple(args...);
    return setLinkCreationPList<Args...>(t);
  }
};

}  // namespace PL
}  // namespace HH
