/*
 * (C) Copyright 2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "ioda/Variables/Has_Variables.h"

#include "ioda/Layout.h"
#include "ioda/Misc/MergeMethods.h"
#include "ioda/Misc/SFuncs.h"

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
FillValuePolicy Has_Variables_Base::getFillValuePolicy() const {
  return backend_->getFillValuePolicy();
}
FillValuePolicy Has_Variables_Backend::getFillValuePolicy() const {
  return FillValuePolicy::NETCDF4;
}

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
  Variable openedVariable = backend_->open(layout_->doMap(name));
  return openedVariable;
}

void Has_Variables_Base::stitchComplementaryVariables(bool removeOriginals) {
  if (layout_->name() != std::string("ObsGroup ODB v1"))
    return;
  std::vector<std::string> variableList = list();
  for (auto const& name : variableList) {
    const std::string destinationName = layout_->doMap(name);
    if (layout_->isComplementary(destinationName)) {
      size_t position = layout_->getComplementaryPosition(destinationName);
      std::string outputName = layout_->getOutputNameFromComponent(destinationName);
      bool oneVariableStitch = false;
      if (layout_->getInputsNeeded(destinationName) == 1)
        oneVariableStitch = true;

      bool outputVariableMetadataPreviouslyGenerated = true;
      // point to the derived variable parameter group if it has already been created (if
      // another component variable has already been accessed)
      auto const it =
          std::find_if(complementaryVariables_.begin(), complementaryVariables_.end(),
                       [&outputName](ComplementaryVariableCreationParameters compParam) {
          return compParam.outputName == outputName; });
      if (it == complementaryVariables_.end()) {
        outputVariableMetadataPreviouslyGenerated = false;
        ComplementaryVariableCreationParameters derivedVariable =
            createDerivedVariableParameters(name, outputName, position);
        complementaryVariables_.push_back(derivedVariable);
      }

      if (outputVariableMetadataPreviouslyGenerated || oneVariableStitch) {
        ComplementaryVariableCreationParameters* derivedVariableParams;
        if (oneVariableStitch) {
          derivedVariableParams = &complementaryVariables_.back();
        } else {
          derivedVariableParams = &(*it);
          derivedVariableParams->inputVariableNames.at(position) = name;
          derivedVariableParams->inputVarsEnteredCount++;
          if (derivedVariableParams->inputVarsEnteredCount !=
              derivedVariableParams->inputVarsNeededCount) {
            continue;
          }
        }
        std::vector<std::vector<std::string> > mergeMethodInput =
            loadComponentVariableData(*derivedVariableParams);
        Variable derivedVariable;
        if (derivedVariableParams->mergeMethod == ioda::detail::DataLayoutPolicy::MergeMethod::Concat) {
          std::vector<std::string> derivedVector = concatenateStringVectors(mergeMethodInput);
          derivedVariable = this->create<std::string>(
                derivedVariableParams->outputName,
                {gsl::narrow<ioda::Dimensions_t>(derivedVector.size())});
          derivedVariable.write(derivedVector);
        }
        if (removeOriginals) {
          for (auto const& inputVar : derivedVariableParams->inputVariableNames) {
            this->remove(layout_->doMap(inputVar));
          }
        }
      }
    }
  }
}

ComplementaryVariableCreationParameters Has_Variables_Base::createDerivedVariableParameters(
    const std::string &inputName, const std::string &outputName, size_t position) {
  ComplementaryVariableCreationParameters newDerivedVariable(outputName);
  const std::string destName = layout_->doMap(inputName);
  newDerivedVariable.mergeMethod = layout_->getMergeMethod(destName);
  newDerivedVariable.inputVarsNeededCount = layout_->getInputsNeeded(destName);
  // Populates a vector with 1 empty entry for every vector that must be entered
  std::vector<std::string> vec(newDerivedVariable.inputVarsNeededCount, "");
  newDerivedVariable.inputVariableNames = std::move(vec);
  newDerivedVariable.inputVariableNames.at(position) = inputName;
  newDerivedVariable.inputVarsEnteredCount = 1;
  return newDerivedVariable;
}

std::vector<std::vector<std::string> > Has_Variables_Base::loadComponentVariableData(
    const ComplementaryVariableCreationParameters &derivedVariableParams)
{
    std::vector<std::vector<std::string>> mergeMethodInput;
    for (size_t inputVariableIdx = 0; inputVariableIdx !=
         derivedVariableParams.inputVarsEnteredCount; inputVariableIdx++) {
      Variable inputVariable = this->open(
            derivedVariableParams.inputVariableNames[inputVariableIdx]);
      mergeMethodInput.push_back(inputVariable.readAsVector<std::string>());
    }
    return mergeMethodInput;
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
  const std::map<BasicTypes, std::function<void(FillValuePolicy, detail::FillValueData_t&)>>
    fvp_map{{BasicTypes::bool_, applyFillValuePolicy<bool>},
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
    backend_->create(layout_->doMap(name), in_memory_dataType, dimensions,
                     max_dimensions, params);
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

VariableCreationParameters& VariableCreationParameters::operator=(
  const VariableCreationParameters& r) {
  if (this == &r) return *this;
  dimsToAttach_        = r.dimsToAttach_;
  dimScaleName_        = r.dimScaleName_;
  fillValue_           = r.fillValue_;
  chunk                = r.chunk;
  chunks               = r.chunks;
  gzip_                = r.gzip_;
  szip_                = r.szip_;
  gzip_level_          = r.gzip_level_;
  szip_PixelsPerBlock_ = r.szip_PixelsPerBlock_;
  szip_options_        = r.szip_options_;
  atts                 = r.atts;
  _py_setFillValue     = decltype(_py_setFillValue){this};
  return *this;
}

bool VariableCreationParameters::hasSetDimScales() const { return (!dimsToAttach_.empty()); }
VariableCreationParameters& VariableCreationParameters::attachDimensionScale(
  unsigned int DimensionNumber, const Variable& scale) {
  dimsToAttach_.emplace_back(std::make_pair(DimensionNumber, scale));
  return *this;
}
VariableCreationParameters& VariableCreationParameters::setDimScale(
  const std::vector<Variable>& vdims) {
  for (unsigned i = 0; i < vdims.size(); ++i) attachDimensionScale(i, vdims[i]);
  return *this;
}
VariableCreationParameters& VariableCreationParameters::setIsDimensionScale(
  const std::string& scaleName) {
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
  szip_       = false;
  gzip_       = true;
  gzip_level_ = level;
}
void VariableCreationParameters::compressWithSZIP(unsigned PixelsPerBlock, unsigned options) {
  gzip_                = false;
  szip_                = true;
  szip_PixelsPerBlock_ = PixelsPerBlock;
  szip_options_        = options;
}

Variable VariableCreationParameters::applyImmediatelyAfterVariableCreation(Variable h) const {
  for (const auto& d : dimsToAttach_) h.attachDimensionScale(d.first, d.second);
  if (isDimensionScale()) h.setIsDimensionScale(getDimensionScaleName());

  atts.apply(h.atts);

  return h;
}
}  // namespace ioda
