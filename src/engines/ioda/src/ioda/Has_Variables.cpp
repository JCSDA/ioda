/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "ioda/Variables/Has_Variables.h"

#include "ioda/Layout.h"

namespace ioda {
namespace detail {
Has_Variables_Base::~Has_Variables_Base() = default;
Has_Variables_Base::Has_Variables_Base(std::shared_ptr<Has_Variables_Backend> b,
                                       std::shared_ptr<const DataLayoutPolicy> layoutPolicy)
    : backend_{b}, layout_{layoutPolicy} {
  if (!layout_) layout_ = DataLayoutPolicy::generate(DataLayoutPolicy::Policies::None);
}
Has_Variables_Backend::~Has_Variables_Backend() = default;
Has_Variables_Backend::Has_Variables_Backend() : Has_Variables_Base(nullptr) {}

void Has_Variables_Base::setLayout(std::shared_ptr<const detail::DataLayoutPolicy> layout) {
  layout_ = layout;
}
FillValuePolicy Has_Variables_Base::getFillValuePolicy() const { return backend_->getFillValuePolicy(); }
FillValuePolicy Has_Variables_Backend::getFillValuePolicy() const { return FillValuePolicy::NETCDF4; }

Type_Provider* Has_Variables_Base::getTypeProvider() const {
  Expects(backend_ != nullptr);
  return backend_->getTypeProvider();
}
bool Has_Variables_Base::exists(const std::string& name) const {
  Expects(backend_ != nullptr);
  Expects(layout_ != nullptr);
  return backend_->exists(layout_->doMap(name));
}
void Has_Variables_Base::remove(const std::string& name) {
  Expects(backend_ != nullptr);
  Expects(layout_ != nullptr);
  backend_->remove(layout_->doMap(name));
}
Variable Has_Variables_Base::open(const std::string& name) const {
  Expects(backend_ != nullptr);
  Expects(layout_ != nullptr);
  return backend_->open(layout_->doMap(name));
}
// This is a one-level search. For searching contents of an ObsGroup, you need to
// list the Variables in each child group.
std::vector<std::string> Has_Variables_Base::list() const {
  Expects(backend_ != nullptr);
  return backend_->list();
}
void Has_Variables_Base::_py_fvp_helper(BasicTypes dataType, FillValuePolicy& fvp,
                                        VariableCreationParameters& params) {
  using namespace FillValuePolicies;
  const std::map<BasicTypes, std::function<void(FillValuePolicy, detail::FillValueData_t&)>> fvp_map{
    {BasicTypes::bool_, applyFillValuePolicy<bool>},
    {BasicTypes::char_, applyFillValuePolicy<char>},
    {BasicTypes::double_, applyFillValuePolicy<double>},
    {BasicTypes::float_, applyFillValuePolicy<float>},
    {BasicTypes::int16_, applyFillValuePolicy<int16_t>},
    {BasicTypes::int32_, applyFillValuePolicy<int32_t>},
    {BasicTypes::int64_, applyFillValuePolicy<int64_t>},
    {BasicTypes::int_, applyFillValuePolicy<int>},
    {BasicTypes::ldouble_, applyFillValuePolicy<long double>},
    {BasicTypes::lint_, applyFillValuePolicy<long int>},
    {BasicTypes::llint_, applyFillValuePolicy<long long int>},
    {BasicTypes::short_, applyFillValuePolicy<short int>},
    {BasicTypes::str_, applyFillValuePolicy<std::string>},
    {BasicTypes::uint16_, applyFillValuePolicy<uint16_t>},
    {BasicTypes::uint32_, applyFillValuePolicy<uint32_t>},
    {BasicTypes::uint64_, applyFillValuePolicy<uint64_t>},
    {BasicTypes::uint_, applyFillValuePolicy<unsigned int>},
    {BasicTypes::ulint_, applyFillValuePolicy<unsigned long int>},
    {BasicTypes::ullint_, applyFillValuePolicy<unsigned long long int>},
    {BasicTypes::ushort_, applyFillValuePolicy<unsigned short int>}};
  if (fvp_map.count(dataType))
    fvp_map.at(dataType)(fvp, params.fillValue_);
  else
    throw;  // jedi_throw.add("Reason", "Unimplemented in map.");
}

Variable Has_Variables_Base::create(const std::string& name, const Type& in_memory_dataType,
                                    const std::vector<Dimensions_t>& dimensions,
                                    const std::vector<Dimensions_t>& max_dimensions,
                                    const VariableCreationParameters& params) {
  Expects(backend_ != nullptr);
  Expects(layout_ != nullptr);
  auto newVar =
    backend_->create(layout_->doMap(name), in_memory_dataType, dimensions, max_dimensions, params);
  params.applyImmediatelyAfterVariableCreation(newVar);
  return newVar;
}

}  // namespace detail

Has_Variables::~Has_Variables() = default;
Has_Variables::Has_Variables() : Has_Variables_Base(nullptr) {}
Has_Variables::Has_Variables(std::shared_ptr<detail::Has_Variables_Backend> b,
                             std::shared_ptr<const detail::DataLayoutPolicy> pol)
    : Has_Variables_Base(b, pol) {}

VariableCreationParameters::VariableCreationParameters() : _py_setFillValue{this} {}
VariableCreationParameters::VariableCreationParameters(const VariableCreationParameters& r)
    : dimsToAttach_{r.dimsToAttach_},
      dimScaleName_{r.dimScaleName_},
      fillValue_{r.fillValue_},
      chunk{r.chunk},
      chunks{r.chunks},
      gzip_{r.gzip_},
      szip_{r.szip_},
      gzip_level_{r.gzip_level_},
      szip_PixelsPerBlock_{r.szip_PixelsPerBlock_},
      szip_options_{r.szip_options_},
      atts{r.atts},
      _py_setFillValue{this} {}

VariableCreationParameters& VariableCreationParameters::operator=(const VariableCreationParameters& r) {
  if (this == &r) return *this;
  dimsToAttach_ = r.dimsToAttach_;
  dimScaleName_ = r.dimScaleName_;
  fillValue_ = r.fillValue_;
  chunk = r.chunk;
  chunks = r.chunks;
  gzip_ = r.gzip_;
  szip_ = r.szip_;
  gzip_level_ = r.gzip_level_;
  szip_PixelsPerBlock_ = r.szip_PixelsPerBlock_;
  szip_options_ = r.szip_options_;
  atts = r.atts;
  _py_setFillValue = decltype(_py_setFillValue){this};
  return *this;
}

bool VariableCreationParameters::hasSetDimScales() const { return (!dimsToAttach_.empty()); }
VariableCreationParameters& VariableCreationParameters::attachDimensionScale(unsigned int DimensionNumber,
                                                                             const Variable& scale) {
  dimsToAttach_.emplace_back(std::make_pair(DimensionNumber, scale));
  return *this;
}
VariableCreationParameters& VariableCreationParameters::setDimScale(const std::vector<Variable>& vdims) {
  for (unsigned i = 0; i < vdims.size(); ++i) attachDimensionScale(i, vdims[i]);
  return *this;
}
VariableCreationParameters& VariableCreationParameters::setIsDimensionScale(const std::string& scaleName) {
  dimScaleName_ = scaleName;
  return *this;
}
bool VariableCreationParameters::isDimensionScale() const { return (!dimScaleName_.empty()); }
std::string VariableCreationParameters::getDimensionScaleName() const { return dimScaleName_; }

void VariableCreationParameters::noCompress() {
  szip_ = false;
  gzip_ = false;
}
void VariableCreationParameters::compressWithGZIP(int level) {
  szip_ = false;
  gzip_ = true;
  gzip_level_ = level;
}
void VariableCreationParameters::compressWithSZIP(unsigned PixelsPerBlock, unsigned options) {
  gzip_ = false;
  szip_ = true;
  szip_PixelsPerBlock_ = PixelsPerBlock;
  szip_options_ = options;
}

Variable VariableCreationParameters::applyImmediatelyAfterVariableCreation(Variable h) const {
  for (const auto& d : dimsToAttach_) h.attachDimensionScale(d.first, d.second);
  if (isDimensionScale()) h.setIsDimensionScale(getDimensionScaleName());

  atts.apply(h.atts);

  return h;
}
}  // namespace ioda
