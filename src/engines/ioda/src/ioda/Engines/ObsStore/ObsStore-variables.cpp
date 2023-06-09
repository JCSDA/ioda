/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_internals_engines_obsstore
 *
 * @{
 * \file ObsStore-variables.cpp
 * \brief Functions for ioda::Variable and ioda::Has_Variables backed by ObsStore
 */
#include "./ObsStore-variables.h"
#include "ioda/Exception.h"

namespace ioda {
namespace Engines {
namespace ObsStore {
//*************************************************************************
// ObsStore_Variable_Backend functions
//*************************************************************************
ObsStore_Variable_Backend::ObsStore_Variable_Backend()  = default;
ObsStore_Variable_Backend::~ObsStore_Variable_Backend() = default;
ObsStore_Variable_Backend::ObsStore_Variable_Backend(std::shared_ptr<ioda::ObsStore::Variable> b)
    : backend_(b) {
  std::shared_ptr<ioda::ObsStore::Has_Attributes> b_atts(backend_->atts);
  atts = Has_Attributes(std::make_shared<ObsStore_HasAttributes_Backend>(b_atts));

  std::shared_ptr<ioda::ObsStore::Has_Attributes> b_impl_atts(backend_->impl_atts);
  impl_atts_ = Has_Attributes(std::make_shared<ObsStore_HasAttributes_Backend>(b_impl_atts));
}

detail::Type_Provider* ObsStore_Variable_Backend::getTypeProvider() const {
  return ObsStore_Type_Provider::instance();
}

Type ObsStore_Variable_Backend::getType() const {
  return Type{std::make_shared<ObsStore_Type>(backend_->dtype()), typeid(ObsStore_Type)};
}

bool ObsStore_Variable_Backend::isA(Type lhs) const {
  auto typeBackend = std::dynamic_pointer_cast<ObsStore_Type>(lhs.getBackend());
  return backend_->isOfType(typeBackend->getType());
}

bool ObsStore_Variable_Backend::hasFillValue() const {
  try {
    return backend_->hasFillValue();
  } catch (const std::bad_cast&) {
    std::throw_with_nested(Exception("Bad cast.", ioda_Here()));
  }
}

ObsStore_Variable_Backend::FillValueData_t ObsStore_Variable_Backend::getFillValue() const {
  try {
    return backend_->getFillValue();
  } catch (const std::bad_cast&) {
    std::throw_with_nested(Exception("Bad cast.", ioda_Here()));
  }
}

std::vector<Dimensions_t> ObsStore_Variable_Backend::getChunkSizes() const {
  if (!impl_atts_.exists("_chunks")) return {};
  std::vector<Dimensions_t> chunks;
  impl_atts_.read<Dimensions_t>("_chunks", chunks);
  return chunks;
}

std::pair<bool, int> ObsStore_Variable_Backend::getGZIPCompression() const {
  if (!impl_atts_.exists("_gzip")) return std::pair<bool, int>(false, 0);
  return std::pair<bool, int>(true, impl_atts_.read<int>("_gzip"));
}

std::tuple<bool, unsigned, unsigned> ObsStore_Variable_Backend::getSZIPCompression() const {
  if (!impl_atts_.exists("_szip")) return std::tuple<bool, unsigned, unsigned>(false, 0, 0);
  std::vector<unsigned> sz;
  impl_atts_.read<unsigned>("_szip", sz);
  Expects(sz.size() == 2);
  return std::tuple<bool, unsigned, unsigned>(true, sz[0], sz[1]);
}

Dimensions ObsStore_Variable_Backend::getDimensions() const {
  // Convert to Dimensions types
  std::vector<Dimensions_t> iodaDims = backend_->get_dimensions();
  std::size_t numElems               = 1;
  for (std::size_t i = 0; i < iodaDims.size(); ++i) {
    numElems *= iodaDims[i];
  }

  // Create and return a Dimensions object
  std::vector<Dimensions_t> iodaMaxDims = backend_->get_max_dimensions();
  auto iodaRank                         = gsl::narrow<Dimensions_t>(iodaDims.size());
  auto iodaNumElems                     = gsl::narrow<Dimensions_t>(numElems);
  Dimensions dims(iodaDims, iodaMaxDims, iodaRank, iodaNumElems);
  return dims;
}

Variable ObsStore_Variable_Backend::resize(const std::vector<Dimensions_t>& newDims) {
  backend_->resize(newDims);
  return Variable{shared_from_this()};
}

Variable ObsStore_Variable_Backend::attachDimensionScale(unsigned int DimensionNumber,
                                                         const Variable& scale) {
  auto scaleBackendBase    = scale.get();
  auto scaleBackendDerived = std::dynamic_pointer_cast<ObsStore_Variable_Backend>(scaleBackendBase);
  backend_->attachDimensionScale(DimensionNumber, scaleBackendDerived->backend_);

  return Variable{shared_from_this()};
}

Variable ObsStore_Variable_Backend::detachDimensionScale(unsigned int DimensionNumber,
                                                         const Variable& scale) {
  auto scaleBackendBase    = scale.get();
  auto scaleBackendDerived = std::dynamic_pointer_cast<ObsStore_Variable_Backend>(scaleBackendBase);
  backend_->detachDimensionScale(DimensionNumber, scaleBackendDerived->backend_);

  return Variable{shared_from_this()};
}

bool ObsStore_Variable_Backend::isDimensionScale() const { return backend_->isDimensionScale(); }

Variable ObsStore_Variable_Backend::setIsDimensionScale(const std::string& dimensionScaleName) {
  backend_->setIsDimensionScale(dimensionScaleName);
  return Variable{shared_from_this()};
}

Variable ObsStore_Variable_Backend::getDimensionScaleName(std::string& res) const {
  backend_->getDimensionScaleName(res);
  return Variable{std::make_shared<ObsStore_Variable_Backend>(*this)};
}

/// Is a dimension scale attached to this Variable in a certain position?
bool ObsStore_Variable_Backend::isDimensionScaleAttached(unsigned int DimensionNumber,
                                                         const Variable& scale) const {
  auto scaleBackendBase    = scale.get();
  auto scaleBackendDerived = std::dynamic_pointer_cast<ObsStore_Variable_Backend>(scaleBackendBase);
  return backend_->isDimensionScaleAttached(DimensionNumber, scaleBackendDerived->backend_);
}

Variable ObsStore_Variable_Backend::write(gsl::span<const char> data,
                                          const Type& in_memory_dataType,
                                          const Selection& mem_selection,
                                          const Selection& file_selection) {
  // Convert to an obs store data type
  auto typeBackend = std::dynamic_pointer_cast<ObsStore_Type>(in_memory_dataType.getBackend());
  const ioda::ObsStore::Type & dtype = typeBackend->getType();
  std::size_t dtype_size = dtype.getSize();

  // Convert to obs store selection
  //
  // Need to record dimension sizes in the ObsStore Selection object.
  // The memory select comes from the frontend, and has its extent_
  // member set to the dimension sizes of the frontend data. In the
  // case where all points are selected, extent_ will be empty so create
  // dimension sizes from the size data.
  //
  // The file select comes from the backend, and gets its dimension
  // sizes from this variable.

  // Copy the selection extent (dim sizes)
  std::size_t extSize = mem_selection.extent().size();
  std::vector<Dimensions_t> dim_sizes(extSize);
  if (extSize == 0) {
    dim_sizes.push_back(data.size() / dtype_size);
  } else {
    for (std::size_t i = 0; i < extSize; ++i) {
      dim_sizes[i] = mem_selection.extent()[i];
    }
  }

  ioda::ObsStore::Selection m_select = createObsStoreSelection(mem_selection, dim_sizes);
  ioda::ObsStore::Selection f_select
    = createObsStoreSelection(file_selection, backend_->get_dimensions());

  // Check the number of points in the selections. Data transfer is going
  // from memory to file so make sure the memory npoints is not greater
  // than the file npoints.
  std::size_t m_npts = m_select.npoints();
  std::size_t f_npts = f_select.npoints();
  if (m_npts > f_npts)
    throw Exception("Number of points from memory is greater than that of file", ioda_Here())
      .add("m_select.npoints()", m_npts)
      .add("f_select.npoints()", f_npts);

  backend_->write(data, dtype, m_select, f_select);
  return Variable{shared_from_this()};
}

Variable ObsStore_Variable_Backend::read(gsl::span<char> data, const Type& in_memory_dataType,
                                         const Selection& mem_selection,
                                         const Selection& file_selection) const {
  // Convert to an obs store data type
  auto typeBackend = std::dynamic_pointer_cast<ObsStore_Type>(in_memory_dataType.getBackend());
  const ioda::ObsStore::Type & dtype = typeBackend->getType();
  std::size_t dtype_size = dtype.getSize();

  // Convert to obs store selection
  //
  // Need to record dimension sizes in the ObsStore Selection object.
  // The memory select comes from the frontend, and has its extent_
  // member set to the dimension sizes of the frontend data. In the
  // case where all points are selected, extent_ will be empty so create
  // dimension sizes from the size data.
  //
  // The file select comes from the backend, and gets its dimension
  // sizes from this variable.

  // Copy the selection extent (dim sizes)
  std::size_t extSize = mem_selection.extent().size();
  std::vector<Dimensions_t> dim_sizes(extSize);
  if (extSize == 0) {
    dim_sizes.push_back(data.size() / dtype_size);
  } else {
    for (std::size_t i = 0; i < extSize; ++i) {
      dim_sizes[i] = mem_selection.extent()[i];
    }
  }

  ioda::ObsStore::Selection m_select = createObsStoreSelection(mem_selection, dim_sizes);
  ioda::ObsStore::Selection f_select
    = createObsStoreSelection(file_selection, backend_->get_dimensions());

  // Check the number of points in the selections. Data transfer is going
  // from file to memory so make sure the file npoints is not greater
  // than the memory npoints.
  std::size_t m_npts = m_select.npoints();
  std::size_t f_npts = f_select.npoints();
  if (m_npts > f_npts)
    throw Exception("Number of points from file is greater than that of memory", ioda_Here())
      .add("m_select.npoints()", m_npts)
      .add("f_select.npoints()", f_npts);

  backend_->read(data, dtype, m_select, f_select);
  // Need to construct a shared_ptr to "this", instead of using
  // shared_from_this() because of the const qualifier on this method.
  return Variable{std::make_shared<ObsStore_Variable_Backend>(*this)};
}

//*************************************************************************
// ObsStore_HasVariables_Backend functions
//*************************************************************************
ObsStore_HasVariables_Backend::ObsStore_HasVariables_Backend() : backend_(nullptr) {}
ObsStore_HasVariables_Backend::ObsStore_HasVariables_Backend(
  std::shared_ptr<ioda::ObsStore::Has_Variables> b)
    : backend_(b) {}
ObsStore_HasVariables_Backend::~ObsStore_HasVariables_Backend() = default;

detail::Type_Provider* ObsStore_HasVariables_Backend::getTypeProvider() const {
  return ObsStore_Type_Provider::instance();
}

bool ObsStore_HasVariables_Backend::exists(const std::string& name) const {
  return backend_->exists(name);
}

void ObsStore_HasVariables_Backend::remove(const std::string& name) { backend_->remove(name); }

Variable ObsStore_HasVariables_Backend::open(const std::string& name) const {
  auto res = backend_->open(name);
  auto b   = std::make_shared<ObsStore_Variable_Backend>(res);
  Variable var{b};
  return var;
}

std::vector<std::string> ObsStore_HasVariables_Backend::list() const { return backend_->list(); }

Variable ObsStore_HasVariables_Backend::create(const std::string& name,
                                               const Type& in_memory_dataType,
                                               const std::vector<Dimensions_t>& dimensions,
                                               const std::vector<Dimensions_t>& max_dimensions,
                                               const VariableCreationParameters& params) {
  // Convert to an obs store data type
  auto typeBackend = std::dynamic_pointer_cast<ObsStore_Type>(in_memory_dataType.getBackend());
  const ioda::ObsStore::Type & dtype = typeBackend->getType();

  // If max_dimensions not specified (empty), then copy from dimensions
  std::vector<Dimensions_t> max_dims;
  if (max_dimensions.empty()) {
    max_dims = dimensions;
  } else {
    max_dims = max_dimensions;
  }

  // Convert to obs store create parameters.
  ioda::ObsStore::VarCreateParams os_params;

  os_params.fvdata        = params.fillValue_;
  const auto fvdata_final = params.finalize();  // Using in a span. Keep in scope.
  if (os_params.fvdata.set_) {
    os_params.fill_value
      = gsl::make_span<char>((char*)&(fvdata_final), sizeof(fvdata_final));  // NOLINT
  }

  // Call backend create
  auto res = backend_->create(name, std::make_shared<ioda::ObsStore::Type>(dtype),
                              dimensions, max_dims, os_params);
  auto b   = std::make_shared<ObsStore_Variable_Backend>(res);

  // Also set the chunking and compression parameters
  if (params.chunk) b->impl_atts_.add<Dimensions_t>("_chunks", params.getChunks(dimensions));
  if (params.gzip_) b->impl_atts_.add<int>("_gzip", params.gzip_level_);
  if (params.szip_)
    b->impl_atts_.add<unsigned>("_szip", {params.szip_options_, params.szip_PixelsPerBlock_});

  Variable var{b};

  return var;
}
}  // namespace ObsStore
}  // namespace Engines
}  // namespace ioda

/// @}
