#pragma once
/*
 * (C) Copyright 2017-2020 Ryan Honeyager (ryan@honeyager.info)
 *
 * This software is licensed under the terms of the BSD 2-Clause License.
 * For details, see the LICENSE file.
 */
#include <hdf5.h>
#include <hdf5_hl.h>

#include <cstring>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "./defs.hpp"
#ifdef __has_include
#if __has_include(<Eigen/Dense>)
#include <Eigen/Dense>
#endif
#endif
#include <gsl/gsl-lite.hpp>

#include "Attributes.hpp"
#include "Errors.hpp"
#include "Funcs.hpp"
#include "Handles.hpp"
#include "Tags.hpp"
#include "Types.hpp"
//#include "PropertyLists.hpp"

/* TODOs:
- Write and read from Eigen objects
- Add column names
- Write complex objects / types
- Compression handling?
- Convenience function to create a property list that sets the fill value, chunking, compression type and
level.
*/

namespace HH {
using namespace HH::Handles;
using namespace HH::Types;
using std::initializer_list;
using std::tuple;

struct HH_DL Dataset {
private:
  HH_hid_t dset;

public:
  Dataset(HH_hid_t hnd_dset = HH::HH_hid_t::dummy());
  virtual ~Dataset();
  HH_hid_t get() const;

  static bool isDataset(HH_hid_t obj);
  inline bool isDataset() const { return isDataset(dset); }

  /// Attributes
  Has_Attributes atts;

  /// Get type
  HH_NODISCARD HH_hid_t getType() const;
  /// Get type
  inline HH_hid_t type() const { return getType(); }

  /// Convenience function to check an dataset's type.
  /// \returns True if the type matches
  /// \returns False (0) if the type does not match
  /// \returns <0 if an error occurred.
  template <class DataType>
  bool isOfType() const {
    auto ttype = HH::Types::GetHDF5Type<DataType>();
    HH_hid_t otype = getType();
    auto ret = H5Tequal(ttype(), otype());
    if (ret < 0) throw;  // HH_throw;
    return (ret > 0) ? true : false;
  }

  /// Convenience function to check an attribute's type.
  /// \param ttype is the type to test against.
  /// \returns True if the type matches
  /// \returns False (0) if the type does not match
  /// \returns <0 if an error occurred.
  bool isOfType(HH_hid_t ttype) const {
    HH_hid_t otype = getType();
    auto ret = H5Tequal(ttype(), otype());
    if (ret < 0) throw;  // HH_throw;
    return (ret > 0) ? true : false;
  }

  // Get dataspace
  HH_NODISCARD HH_hid_t getSpace() const;

  /// Get current and maximum dimensions, and number of total points.
  struct Dimensions {
    std::vector<hsize_t> dimsCur, dimsMax;
    hsize_t dimensionality;
    hsize_t numElements;
    Dimensions(const std::vector<hsize_t>& dimscur, const std::vector<hsize_t>& dimsmax, hsize_t dality,
               hsize_t np)
        : dimsCur(dimscur), dimsMax(dimsmax), dimensionality(dality), numElements(np) {}
  };
  Dimensions getDimensions() const;

  // Resize the dataset?

  /// \brief Write the dataset
  /// \note Ensure that the correct dimension ordering is preserved.
  /// \note With default parameters, the entire dataset is written.
  Dataset writeDirect(const gsl::span<const char> data, HH_hid_t in_memory_dataType,
                      HH_hid_t mem_space_id = H5S_ALL, HH_hid_t file_space_id = H5S_ALL,
                      HH_hid_t xfer_plist_id = H5P_DEFAULT) {
    HH_Expects(isDataset());
    auto ret = H5Dwrite(dset(),                // dataset id
                        in_memory_dataType(),  // mem_type_id
                        mem_space_id(),        // mem_space_id
                        file_space_id(),       // file_space_id
                        xfer_plist_id(),       // xfer_plist_id
                        data.data()            // data
    );
    if (ret < 0) throw;  // HH_throw;
    return *this;
  }
  /// \brief Write the dataset
  /// \note Ensure that the correct dimension ordering is preserved.
  /// \note With default parameters, the entire dataset is written.
  template <class DataType, class Marshaller = HH::Types::Object_Accessor<DataType> >
  Dataset write(const gsl::span<const DataType> data,
                HH_hid_t in_memory_dataType = HH::Types::GetHDF5Type<DataType>(),
                HH_hid_t mem_space_id = H5S_ALL, HH_hid_t file_space_id = H5S_ALL,
                HH_hid_t xfer_plist_id = H5P_DEFAULT) {
    HH_Expects(isDataset());
    Marshaller m;
    auto d = m.serialize(data);
    auto ret = H5Dwrite(dset(),                 // dataset id
                        in_memory_dataType(),   // mem_type_id
                        mem_space_id(),         // mem_space_id
                        file_space_id(),        // file_space_id
                        xfer_plist_id(),        // xfer_plist_id
                        d->DataPointers.data()  // data
                                                // data.data() // data
    );
    if (ret < 0) throw;  // HH_throw;
    return *this;
  }
  template <class DataType, class Marshaller = HH::Types::Object_Accessor<DataType> >
  Dataset write(std::initializer_list<const DataType> data,
                HH_hid_t in_memory_dataType = HH::Types::GetHDF5Type<DataType>(),
                HH_hid_t mem_space_id = H5S_ALL, HH_hid_t file_space_id = H5S_ALL,
                HH_hid_t xfer_plist_id = H5P_DEFAULT) {
    return write<DataType, Marshaller>(gsl::span<const DataType>(data.begin(), data.size()),
                                       in_memory_dataType, mem_space_id, file_space_id, xfer_plist_id);
  }
  template <class DataType, class Marshaller = HH::Types::Object_Accessor<DataType> >
  Dataset write(const gsl::span<const DataType> data, const std::vector<hsize_t>& start,
                const std::vector<hsize_t>& count, const std::vector<hsize_t>& stride = {},
                const std::vector<hsize_t>& block = {}, HH_hid_t xfer_plist_id = H5P_DEFAULT,
                HH_hid_t in_memory_dataType = HH::Types::GetHDF5Type<DataType>()) {
    const hsize_t* pStride = (stride.size()) ? stride.data() : nullptr;
    const hsize_t* pBlock = (block.size()) ? block.data() : nullptr;

    /*
    auto sz = getDimensions();
    HH_Expects(sz.dimensionality == start.size());
    HH_Expects(sz.dimensionality == count.size());
    if (stride.size()) HH_Expects(sz.dimensionality == stride.size());
    if (block.size()) HH_Expects(sz.dimensionality == block.size());
    for (size_t i = 0; i < sz.dimensionality; ++i) {
            HH_Expects(start[i] < sz.dimsCur[i]);
            HH_Expects(start[i]+count[i] < sz.dimsCur[i]);
    }
    */

    auto dSpaceFull = getSpace();

    auto dSpaceToSelectionMem = HH::HH_hid_t(H5Scopy(dSpaceFull()), HH::Closers::CloseHDF5Dataspace::CloseP);
    std::vector<hsize_t> memStart(start.size());
    auto res_shm = H5Sselect_hyperslab(dSpaceToSelectionMem(), H5S_SELECT_SET,
                                       memStart.data(),  // start
                                       nullptr,          // stride
                                       count.data(),     // count
                                       nullptr           // block size
    );
    // auto res_shm = H5Sselect_hyperslab(dSpaceToSelectionMem(), H5S_SELECT_SET,
    //	start.data(),				// start
    //	nullptr,					// stride
    //	count.data(),	// count
    //	nullptr						// block size
    //);
    if (res_shm < 0) throw;  // HH_throw;

    auto dSpaceSelection = HH::HH_hid_t(H5Scopy(dSpaceFull()), Closers::CloseHDF5Dataspace::CloseP);
    if (H5Sselect_hyperslab(dSpaceSelection(), H5S_SELECT_SET, start.data(), pStride, count.data(), pBlock) <
        0)
      throw;  // HH_throw;

    return write<DataType, Marshaller>(data, in_memory_dataType, dSpaceToSelectionMem, dSpaceSelection,
                                       xfer_plist_id);
  }

  /*
  /// \brief Read the dataset
  /// \note Ensure that the correct dimension ordering is preserved
  /// \note With default parameters, the entire dataset is read
  inline Dataset read(
          gsl::span<char> data,
          HH_hid_t in_memory_dataType,
          HH_hid_t mem_space_id = H5S_ALL,
          HH_hid_t file_space_id = H5S_ALL,
          HH_hid_t xfer_plist_id = H5P_DEFAULT) const
  {
          HH_Expects(isDataset());
          auto ret = H5Dread(
                  dset(), // dataset id
                  in_memory_dataType(), // mem_type_id
                  mem_space_id(), // mem_space_id
                  file_space_id(), // file_space_id
                  xfer_plist_id(), // xfer_plist_id
                  data.data() // data
          );
          if (ret < 0) throw;  // HH_throw;
          return *this;
  }
  */

  Dataset readDirect(gsl::span<char> data, HH_hid_t in_memory_dataType, HH_hid_t mem_space_id = H5S_ALL,
                     HH_hid_t file_space_id = H5S_ALL, HH_hid_t xfer_plist_id = H5P_DEFAULT) const {
    herr_t ret = H5Dread(dset(), in_memory_dataType(), mem_space_id(), file_space_id(), xfer_plist_id(),
                         reinterpret_cast<void*>(data.data()));
    if (ret < 0) throw;  // HH_throw.add("Reason", "H5Dread failed.");
    return *this;
  }

  template <class DataType, class Marshaller = HH::Types::Object_Accessor<DataType> >
  Dataset _read(HH_hid_t in_memory_dataType, size_t flsize, gsl::span<DataType> data, bool isvlen,
                HH_hid_t mem_space_id = H5S_ALL, HH_hid_t file_space_id = H5S_ALL,
                HH_hid_t xfer_plist_id = H5P_DEFAULT) const {
    Marshaller m;
    auto p = m.prep_deserialize(data.length_bytes());  // data.length_bytes()? // flsize
    herr_t ret = H5Dread(dset(), in_memory_dataType(), mem_space_id(), file_space_id(), xfer_plist_id(),
                         reinterpret_cast<void*>(p->DataPointers.data()));
    if (ret < 0) throw;  // HH_throw.add("Reason", "H5Dread failed.");
    m.deserialize(p, data);
    return *this;
  }

  template <class DataType>
  Dataset read(gsl::span<DataType> data, HH_hid_t in_memory_dataType = HH::Types::GetHDF5Type<DataType>(),
               HH_hid_t mem_space_id = H5S_ALL, HH_hid_t file_space_id = H5S_ALL,
               HH_hid_t xfer_plist_id = H5P_DEFAULT) const {
    HH_Expects(isDataset());
    auto ftype = getType();
    H5T_class_t type_class = H5Tget_class(ftype());
    // Detect if this is a variable-length array string type:
    bool isVLenArrayType = false;
    if (type_class == H5T_STRING) {
      htri_t i = H5Tis_variable_str(ftype());
      if (i > 0) isVLenArrayType = true;
    }
    hsize_t flsize = getDimensions().numElements;

    _read(in_memory_dataType, flsize, data, isVLenArrayType, mem_space_id, file_space_id, xfer_plist_id);
    return *this;
  }

#if __has_include(<Eigen/Dense>)
  template <class EigenClass>
  Dataset readWithEigen(EigenClass&& res, bool resize = true) const {
    typedef typename EigenClass::Scalar ScalarType;
    HH_hid_t dtype = HH::Types::GetHDF5Type<ScalarType>();
    // Check that the dims are 1 or 2.
    auto dims = getDimensions();
    if (resize)
      if (dims.dimensionality > 2) throw;  // HH_throw;
    int nDims[2] = {1, 1};
    if (dims.dimsCur.size() >= 1) nDims[0] = gsl::narrow<int>(dims.dimsCur[0]);
    if (dims.dimsCur.size() >= 2) nDims[1] = gsl::narrow<int>(dims.dimsCur[1]);

    if (resize)
      res.resize(nDims[0], nDims[1]);
    else if (dims.numElements != (size_t)(res.rows() * res.cols()))
      throw;  // HH_throw;

    // Array copy to preserve row vs column major format.
    /// \todo Implement this more efficiently (i.e. only when needed).
    Eigen::Array<ScalarType, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> data_in;
    data_in.resize(res.rows(), res.cols());

    herr_t e = read<ScalarType>(gsl::span<ScalarType>(data_in.data(), dims.numElements));
    if (e < 0) throw;  // HH_throw;
    res = data_in;
    return *this;
  }

  template <class EigenClass>
  Dataset readWithEigen(EigenClass& res, bool resize = true) const {
    typedef typename EigenClass::Scalar ScalarType;
    HH_hid_t dtype = HH::Types::GetHDF5Type<ScalarType>();
    // Check that the dims are 1 or 2.
    auto dims = getDimensions();
    if (resize)
      if (dims.dimensionality > 2) throw;  // HH_throw;
    int nDims[2] = {1, 1};
    if (dims.dimsCur.size() >= 1) nDims[0] = gsl::narrow<int>(dims.dimsCur[0]);
    if (dims.dimsCur.size() >= 2) nDims[1] = gsl::narrow<int>(dims.dimsCur[1]);

    if (resize)
      res.resize(nDims[0], nDims[1]);
    else if (dims.numElements != (size_t)(res.rows() * res.cols()))
      throw;  // HH_throw;

    // Array copy to preserve row vs column major format.
    /// \todo Implement this more efficiently (i.e. only when needed).
    Eigen::Array<ScalarType, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> data_in;
    data_in.resize(res.rows(), res.cols());

    read<ScalarType>(gsl::span<ScalarType>(data_in.data(), dims.numElements));

    res = data_in;
    return *this;
  }

  template <class EigenClass>
  Dataset writeWithEigen(const EigenClass& d, const std::vector<hsize_t>& start = {},
                         HH_hid_t xfer_plist_id = H5P_DEFAULT,
                         HH_hid_t dtype = HH::Types::GetHDF5Type<typename EigenClass::Scalar>()) {
    typedef typename EigenClass::Scalar ScalarType;
    // HH_hid_t dtype = HH::Types::GetHDF5Type<ScalarType>();
    /// \todo Handle the different row and column major formats for output more efficiently.
    Eigen::Array<ScalarType, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> dout;
    dout.resize(d.rows(), d.cols());
    dout = d;
    const auto& dconst = dout;
    auto sp = gsl::make_span(dconst.data(), gsl::narrow<int>(d.rows() * d.cols()));

    size_t dimensionality = getDimensions().dimensionality;
    std::vector<hsize_t> sstart = start;
    if (!sstart.size()) sstart = std::vector<hsize_t>(dimensionality, 0);
    std::vector<hsize_t> scount;
    if (dimensionality == 1)
      scount.push_back(gsl::narrow<size_t>(d.rows() * d.cols()));
    else if (dimensionality == 2) {
      scount.push_back(gsl::narrow<size_t>(d.rows()));
      scount.push_back(gsl::narrow<size_t>(d.cols()));
    } else
      throw;  // HH_throw;
    return write(sp, sstart, scount, {}, {}, xfer_plist_id, dtype);
  }

  template <class EigenClass>
  Dataset writeWithEigenTensor(const EigenClass& d, const std::vector<hsize_t>& start = {},
                               HH_hid_t xfer_plist_id = H5P_DEFAULT,
                               HH_hid_t dtype = HH::Types::GetHDF5Type<typename EigenClass::Scalar>()) {
    typedef typename EigenClass::Scalar ScalarType;
    int numDims = d.NumDimensions;
    const auto& dims = d.dimensions();
    std::vector<hsize_t> hdims;
    for (const auto& dim : dims) hdims.push_back(gsl::narrow<hsize_t>(dim));
    auto sz = d.size();

    auto sp = (gsl::make_span(d.data(), sz));
    auto res = write(sp);
    return res;
  }

#endif

  /// Attach a dimension scale to this table.
  HH_MAYBE_UNUSED Dataset attachDimensionScale(unsigned int DimensionNumber, const Dataset& scale);
  /// Detach a dimension scale
  HH_MAYBE_UNUSED Dataset detachDimensionScale(unsigned int DimensionNumber, const Dataset& scale);
  HH_MAYBE_UNUSED Dataset setDims(std::initializer_list<Dataset> dims);
  HH_MAYBE_UNUSED Dataset setDims(const Dataset& dims);
  HH_MAYBE_UNUSED Dataset setDims(const Dataset& dim1, const Dataset& dim2);
  HH_MAYBE_UNUSED Dataset setDims(const Dataset& dim1, const Dataset& dim2, const Dataset& dim3);
  /*
  template <typename AttType>
  Dataset AddSimpleAttributes(const std::pair<std::string, AttType> &att)
  {
  atts.add<AttType>(att.first, att.second);
  return *this;
  }
  template <typename AttType, typename... Rest>
  Dataset AddSimpleAttributes(const std::pair<std::string, AttType> &att, Rest&&... rest)
  {
  AddSimpleAttributes(att);
  AddSimpleAttributes<Rest...>(std::forward<Rest>(rest)...);
  return *this;
  }
  template <typename AttType>
  Dataset AddSimpleAttributes(const std::string &attname, AttType val)
  {
  atts.add<AttType>(attname.c_str(), val);
  return *this;
  }
  */

  /// \note NameType is unused; should always be const char*. Exists to make my template formatting easier.
  template <typename NameType, typename AttType>
  HH_MAYBE_UNUSED Dataset AddSimpleAttributes(NameType attname, AttType val) {
    HH_Expects(isDataset());
    atts.add<AttType>(attname, {val}, {1});
    /// \todo Check return value
    return *this;
  }
  template <typename NameType, typename AttType, typename... Rest>
  HH_MAYBE_UNUSED Dataset AddSimpleAttributes(NameType attname, AttType val, Rest... rest) {
    AddSimpleAttributes<NameType, AttType>(attname, val);
    AddSimpleAttributes<Rest...>(rest...);
    return *this;
  }

  /// Is this dataset used as a dimension scale?
  bool isDimensionScale() const;

  /// Designate this table as a dimension scale
  HH_MAYBE_UNUSED Dataset setIsDimensionScale(const std::string& dimensionScaleName);
  /// Set the axis label for the dimension designated by DimensionNumber
  HH_MAYBE_UNUSED Dataset setDimensionScaleAxisLabel(unsigned int DimensionNumber, const std::string& label);
  /// \brief Get the axis label for the dimension designated by DimensionNumber
  /// \todo See if there is ANY way to dynamically determine the label size. HDF5 docs do not discuss this.
  std::string getDimensionScaleAxisLabel(unsigned int DimensionNumber) const;
  HH_MAYBE_UNUSED Dataset getDimensionScaleAxisLabel(unsigned int DimensionNumber, std::string& res) const;
  /// \brief Get the name of this table's defined dimension scale
  /// \todo See if there is ANY way to dynamically determine the label size. HDF5 docs do not discuss this.
  std::string getDimensionScaleName() const;
  HH_MAYBE_UNUSED Dataset getDimensionScaleName(std::string& res) const;

  /// Is a dimension scale attached to this dataset in a certain position?
  bool isDimensionScaleAttached(const Dataset& scale, unsigned int DimensionNumber) const;
};

inline bool Chunking_Max(const std::vector<hsize_t>& in, std::vector<hsize_t>& out) {
  out = in;
  return true;
}

HH_DL std::pair<bool, bool> isFilteravailable(H5Z_filter_t filt);

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

/** \brief Order-obeying filter insertions and replacements

Filters will be repeatedly removed and reinserted to get the desired filter order.
The desired filter order is:
- Shuffling
- Compression

Life would be easier if HDF5 allowed for an easy way to insert filters at specified orderings.
**/
struct HH_DL Filters {
private:
  HH_hid_t pl;

public:
  Filters(HH_hid_t newbase);
  virtual ~Filters();
  /// \see https://support.hdfgroup.org/HDF5/doc/RM/RM_H5P.html#Property-SetFilter for meanings
  struct filter_info {
    H5Z_filter_t id = -1;
    unsigned int flags = 0;
    std::vector<unsigned int> cd_values;
  };
  /// Get a vector of the filters that are implemented
  std::vector<filter_info> get() const;
  /// Append the filters to a property list.
  void append(const std::vector<filter_info>& filters);
  /// Set the filters to a property list. Clears existing filters.
  void set(const std::vector<filter_info>& filters);
  void clear();

  enum class FILTER_T { SHUFFLE, COMPRESSION, SCALE, OTHER };

  bool has(H5Z_filter_t id) const;

  static FILTER_T getType(const filter_info& it);
  static bool isA(const filter_info& it, FILTER_T typ);
  void appendOfType(const std::vector<filter_info>& filters, FILTER_T typ);
  void removeOfType(FILTER_T typ);

  void setShuffle();
  void setSZIP(unsigned int optm, unsigned int ppb);
  void setGZIP(unsigned int level);
  void setScaleOffset(H5Z_SO_scale_type_t scale_type, int scale_factor);
};

struct HH_DL DatasetParameterPack {
private:
  std::vector<std::pair<unsigned int, Dataset> > _dimsToAttach;

public:
  AttributeParameterPack atts;

  struct DatasetCreationPListProperties {
    bool chunk = false;
    bool shuffle = false;
    // bool compress = false;
    bool gzip = false;
    bool szip = false;
    int gzip_level = 6;
    unsigned int szip_PixelsPerBlock = 16;
    unsigned int szip_options = H5_SZIP_EC_OPTION_MASK;
    bool hasFillValue = false;
    union {
      int64_t i64;
      uint64_t ui64;
      double d;
      float f;
      int32_t i32;
      uint32_t ui32;
      int16_t i16;
      uint16_t ui16;
      char c;
      unsigned char uc;
      long double ld;
    } fillValue = {0};
    bool scale = false;
    int scale_factor = 1;
    H5Z_SO_scale_type_t scale_type = H5Z_SO_FLOAT_DSCALE;
    HH_hid_t fillValue_type = HH_hid_t::dummy();
    template <class DataType>
    DatasetCreationPListProperties& setFill(DataType fill) {
      hasFillValue = true;
      fillValue_type = HH::Types::GetHDF5Type<DataType>();
      memcpy(&(fillValue.ld), &fill, sizeof(fill));
      // fillValue.ui64 = static_cast<uint64_t>(fill);
      return *this;
    }

    HH_hid_t generate(const std::vector<hsize_t>& chunkingBlockSize) const;
  } datasetCreationProperties;

  std::function<bool(const std::vector<hsize_t>&, std::vector<hsize_t>&)> fChunkingStrategy = Chunking_Max;
  std::vector<hsize_t> customChunkSizes;

  bool UseCustomDatasetCreationPlist = false;
  HH_hid_t DatasetCreationPlistCustom = H5P_DEFAULT;

  HH_hid_t LinkCreationPlist = H5P_DEFAULT;
  HH_hid_t DatasetAccessPlist = H5P_DEFAULT;

  /// Attach a dimension scale to this table.
  HH_MAYBE_UNUSED DatasetParameterPack& attachDimensionScale(unsigned int DimensionNumber,
                                                             const Dataset& scale);
  HH_MAYBE_UNUSED DatasetParameterPack& setDims(std::initializer_list<Dataset> dims);
  HH_MAYBE_UNUSED DatasetParameterPack& setDims(const Dataset& dims);
  HH_MAYBE_UNUSED DatasetParameterPack& setDims(const Dataset& dim1, const Dataset& dim2);
  HH_MAYBE_UNUSED DatasetParameterPack& setDims(const Dataset& dim1, const Dataset& dim2,
                                                const Dataset& dim3);

  HH_MAYBE_UNUSED Dataset apply(HH::HH_hid_t h) const;

  HH_hid_t generateDatasetCreationPlist(const std::vector<hsize_t>& dims) const;

  DatasetParameterPack();
  DatasetParameterPack(const AttributeParameterPack& a, HH_hid_t LinkCreationPlist = H5P_DEFAULT,
                       HH_hid_t DatasetAccessPlist = H5P_DEFAULT);
};

struct HH_DL Has_Datasets {
private:
  HH_hid_t base;

public:
  Has_Datasets(HH_hid_t obj);
  virtual ~Has_Datasets();

  /// \brief Does a dataset with the specified name exist?
  /// This checks for a link with the given name, and checks that the link is a dataset.
  bool exists(const std::string& dsetname, HH_hid_t LinkAccessPlist = H5P_DEFAULT) const;

  // Remove by name - handled by removing the link
  void remove(const std::string& name);

  // Rename - handled by a new hard link

  /// \brief Open a dataset
  Dataset open(const std::string& dsetname, HH_hid_t DatasetAccessPlist = H5P_DEFAULT) const;

  Dataset operator[](const std::string& dsetname) const;

  /// \brief List all datasets under this group
  std::vector<std::string> list() const;

  /// \brief Open all datasets under the group. Convenience function
  std::map<std::string, Dataset> openAll() const;

private:
  template <class DataType>
  Dataset _create(const std::string& dsetname, const std::vector<hsize_t>& dimensions,
                  const std::vector<hsize_t>& max_dimensions = {},
                  const DatasetParameterPack& parampack = DatasetParameterPack(),
                  HH_hid_t dtype = HH::Types::GetHDF5Type<DataType>()) {
    std::vector<hsize_t> hdims = dimensions;
    std::vector<hsize_t> hmaxdims = max_dimensions;
    if (!hmaxdims.size()) hmaxdims = hdims;

    HH_hid_t dspace{[&]() {
                      auto ret =
                        H5Screate_simple(gsl::narrow<int>(dimensions.size()), hdims.data(), hmaxdims.data());
                      if (ret < 0) throw;  // HH_throw;
                      return ret;
                    }(),
                    Closers::CloseHDF5Dataspace::CloseP};

    auto dcp = parampack.generateDatasetCreationPlist(hdims);

    hid_t dsetid = H5Dcreate(base(), dsetname.c_str(), dtype(), dspace(), parampack.LinkCreationPlist(),
                             dcp(), parampack.DatasetAccessPlist());
    if (dsetid < 0) throw;  // HH_throw;
    HH_hid_t hh(dsetid, Closers::CloseHDF5Dataset::CloseP);
    parampack.apply(hh);
    return Dataset(hh);
  }

public:
  /// \brief Create a dataset
  template <class DataType>
  Dataset create(const std::string& dsetname, std::vector<hsize_t> dimensions,
                 std::vector<hsize_t> max_dimensions = {},
                 const DatasetParameterPack& parampack = DatasetParameterPack(),
                 HH_hid_t dtype = HH::Types::GetHDF5Type<DataType>()) {
    return _create<DataType>(dsetname, dimensions, max_dimensions, parampack, dtype);
  }

#if __has_include(<Eigen/Dense>)
  template <class EigenClass>
  Dataset createWithEigen(const std::string& dsetname, const EigenClass& d,
                          const DatasetParameterPack& parampack = DatasetParameterPack(), int nDims = -1,
                          std::vector<hsize_t> max_dimensions = {},
                          HH_hid_t dtype = HH::Types::GetHDF5Type<typename EigenClass::Scalar>()) {
    typedef typename EigenClass::Scalar ScalarType;
    // HH_hid_t dtype = HH::Types::GetHDF5Type<ScalarType>();
    /// \todo Handle the different row and column major formats for output more efficiently.
    Eigen::Array<ScalarType, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> dout;
    dout.resize(d.rows(), d.cols());
    dout = d;
    const auto& dconst = dout;
    int nRows = static_cast<int>(d.rows());
    int nCols = static_cast<int>(d.cols());
    if (nRows * nCols != static_cast<int>(d.rows() * d.cols())) throw;  // HH_throw;
    if (nDims == -1) nDims = 2;

    if (nDims == 1) {
      auto obj =
        _create<ScalarType>(dsetname, {(size_t)nRows * (size_t)nCols}, max_dimensions, parampack, dtype);
      auto sp = gsl::make_span(dconst.data(), static_cast<int>(nRows * nCols));
      // htri_t res = obj.write< typename EigenClass::Scalar >(sp);
      return obj.write(sp);
    } else if (nDims == 2) {
      // Object like obj[x][y], where rows are x and cols are y.
      auto obj = _create<ScalarType>(dsetname, {static_cast<size_t>(nRows), static_cast<size_t>(nCols)},
                                     max_dimensions, parampack, dtype);
      // auto res = obj.write<ScalarType>(gsl::make_span(dout.data(), dout.rows()*dout.cols()));
      auto sp = (gsl::make_span(dconst.data(), static_cast<int>(nRows * nCols)));
      return obj.write(sp);
    }
    HH_Expects(nDims <= 2);
    HH_Expects(nDims > 0);
    HH_Unimplemented;  // Should return before this.
  }

  template <class EigenClass>
  Dataset createWithEigenTensor(const std::string& dsetname, const EigenClass& d,
                                const DatasetParameterPack& parampack = DatasetParameterPack(),
                                std::vector<hsize_t> max_dimensions = {},
                                HH_hid_t dtype = HH::Types::GetHDF5Type<EigenClass::Scalar>()) {
    typedef typename EigenClass::Scalar ScalarType;
    int numDims = d.NumDimensions;
    const auto& dims = d.dimensions();
    std::vector<hsize_t> hdims;
    for (const auto& dim : dims) hdims.push_back(gsl::narrow<hsize_t>(dim));
    auto sz = d.size();

    // Object like obj[x][y], where rows are x and cols are y.
    auto obj = _create<ScalarType>(dsetname, hdims, max_dimensions, parampack, dtype);
    // auto res = obj.write<ScalarType>(gsl::make_span(dout.data(), dout.rows()*dout.cols()));
    auto sp = (gsl::make_span(d.data(), sz));
    auto res = obj.write(sp);
    if (res < 0) throw;  // HH_throw;
    return obj;
  }
#endif

  template <class DataType>
  Dataset createFromSpan(const std::string& dsetname, const gsl::span<const DataType> d,
                         std::vector<hsize_t> dims = {}, std::vector<hsize_t> max_dimensions = {},
                         const DatasetParameterPack& parampack = DatasetParameterPack(),
                         HH_hid_t dtype = HH::Types::GetHDF5Type<DataType>()) {
    if (!dims.size()) dims.push_back(d.size());
    HH_Expects(dims.size() < 3);
    HH::Dataset obj = HH::Handles::HH_hid_t::dummy();
    std::vector<hsize_t> vdims(dims.begin(), dims.end());
    if (vdims.size() == 1)
      if (vdims[0] == -1) vdims[0] = d.size();

    hsize_t p = 1;
    for (const auto& v : vdims) {
      HH_Expects(v > 0);
      p *= v;
    }
    HH_Expects(p == gsl::narrow<hsize_t>(d.size()));
    obj = create<DataType>(dsetname, std::vector<hsize_t>(vdims.begin(), vdims.end()), max_dimensions,
                           parampack, dtype);

    auto res = obj.write(d);
    return res;
  }
};

/*
namespace Convenience {
        inline hsize_t getMinSize(const std::vector<hsize_t> &sizes)
        {
                return 0;
        }
        inline hsize_t getMinSize(
                const std::vector<std::pair<Dataset, hsize_t> > &tgts,
                hsize_t axis)
        {
                hsize_t minSize = -1;
                hsize_t runningOffset = 0;
                for (const auto &t : tgts) {
                        hsize_t maxOffset = -1;
                        if (t.first.isDataset()) {
                                HH_Expects(t.first.getDimensions().dimensionality > axis);
                                auto sizes = t.first.getDimensions().dimsCur;
                                if (t.second >= 0) maxOffset = sizes[axis] + t.second;
                                else maxOffset = runningOffset + sizes[axis];
                                runningOffset += maxOffset;
                        }
                        else {
                                runningOffset += 0;
                        }

                        if (maxOffset > minSize) minSize = maxOffset;
                }
                return minSize;
        }

        /// Resize dataset to accomodate
        /// \param toMerge takes a pair of Dataset, offset.
        inline Dataset merge(
                Dataset target,
                const std::vector<std::pair<Dataset, hsize_t> > &toMerge,
                hsize_t mergeAxis)
        {
                hsize_t minAxisSize = getMinSize(toMerge, mergeAxis);
                return target;
        }

        inline Dataset merge(
                HH_hid_t outBase,
                const std::string &outDsetName,
                const std::vector<Dataset> &toMerge,
                hsize_t mergeAxis)
        {
        }

}
*/
}  // namespace HH
