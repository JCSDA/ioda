/*
 * (C) Copyright 2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "ioda/Variables/Has_Variables.h"

#include "ioda/Exception.h"
#include "ioda/Layout.h"
#include "ioda/Misc/DimensionScales.h"
#include "ioda/Misc/MergeMethods.h"
#include "ioda/Misc/StringFuncs.h"
#include "ioda/Misc/UnitConversions.h"

#include <stdexcept>

namespace ioda {
namespace detail {

Has_Variables_Base::~Has_Variables_Base() = default;

Has_Variables_Base::Has_Variables_Base(std::shared_ptr<Has_Variables_Backend> b,
                                       std::shared_ptr<const DataLayoutPolicy> layoutPolicy)
    : backend_{b}, layout_{layoutPolicy}
{
  try {
    if (!layout_) layout_ = DataLayoutPolicy::generate(DataLayoutPolicy::Policies::None);
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred in ioda in Has_Variables_Base's constructor.", ioda_Here()));
  }
}

Has_Variables_Backend::~Has_Variables_Backend() = default;

Has_Variables_Backend::Has_Variables_Backend() : Has_Variables_Base(nullptr) {}

void Has_Variables_Base::setLayout(std::shared_ptr<const detail::DataLayoutPolicy> layout) {
  layout_ = layout;
}

FillValuePolicy Has_Variables_Base::getFillValuePolicy() const {
  try {
    if (backend_ == nullptr)
      throw Exception("Missing backend or unimplemented backend function.", ioda_Here());
    return backend_->getFillValuePolicy();
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred in ioda while determining the fill value policy of a backend.", 
      ioda_Here()));
  }
}

FillValuePolicy Has_Variables_Backend::getFillValuePolicy() const {
  return FillValuePolicy::NETCDF4;
}

Type_Provider* Has_Variables_Base::getTypeProvider() const {
  try {
    if (backend_ == nullptr)
      throw Exception("Missing backend or unimplemented backend function.", ioda_Here());
    return backend_->getTypeProvider();
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred in ioda while getting a backend's type provider interface.",
      ioda_Here()));
  }
}

bool Has_Variables_Base::exists(const std::string& name) const {
  try {
    if (backend_ == nullptr)
      throw Exception("Missing backend or unimplemented backend function.", ioda_Here());
    if (layout_ == nullptr)
      throw Exception("Missing layout.", ioda_Here());
    return backend_->exists(layout_->doMap(name));
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while checking variable existence.", ioda_Here())
      .add("name", name));
  }
}

void Has_Variables_Base::remove(const std::string& name) {
  try {
    if (backend_ == nullptr)
      throw Exception("Missing backend or unimplemented backend function.", ioda_Here());
    if (layout_ == nullptr)
      throw Exception("Missing layout.", ioda_Here());
    backend_->remove(layout_->doMap(name));
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while removing a variable.", ioda_Here())
      .add("name", name));
  }
}

Variable Has_Variables_Base::open(const std::string& name) const {
  try {
    if (backend_ == nullptr)
      throw Exception("Missing backend or unimplemented backend function.", ioda_Here());
    if (layout_ == nullptr)
      throw Exception("Missing layout.", ioda_Here());
    return backend_->open(layout_->doMap(name));
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while opening a variable.", ioda_Here())
      .add("name", name));
  }
}

void Has_Variables_Base::stitchComplementaryVariables(bool removeOriginals) {
  try {
    if (layout_->name() != std::string("ObsGroup ODB v1")) return;
    std::vector<std::string> variableList = list();
    // Unique pointer used for lazy initialization. std::optional is not in C++14 and we don't want
    // to introduce a dependency on Boost.
    std::unique_ptr<std::list<Named_Variable>> dimScales;
    for (auto const& name : variableList) {
      const std::string destinationName = layout_->doMap(name);
      if (layout_->isComplementary(destinationName)) {
        size_t position        = layout_->getComplementaryPosition(destinationName);
        std::string outputName = layout_->getOutputNameFromComponent(destinationName);
        bool oneVariableStitch = false;
        if (layout_->getInputsNeeded(destinationName) == 1) oneVariableStitch = true;

        bool outputVariableMetadataPreviouslyGenerated = true;
        // point to the derived variable parameter group if it has already been created (if
        // another component variable has already been accessed)
        auto const it
          = std::find_if(complementaryVariables_.begin(), complementaryVariables_.end(),
                         [&outputName](ComplementaryVariableCreationParameters compParam) {
                           return compParam.outputName == outputName;
                         });
        if (it == complementaryVariables_.end()) {
          outputVariableMetadataPreviouslyGenerated = false;
          ComplementaryVariableCreationParameters derivedVariable
            = createDerivedVariableParameters(name, outputName, position);
          complementaryVariables_.push_back(derivedVariable);
        }

        if (outputVariableMetadataPreviouslyGenerated || oneVariableStitch) {
          ComplementaryVariableCreationParameters* derivedVariableParams;
          if (oneVariableStitch) {
            derivedVariableParams = &complementaryVariables_.back();
          } else {
            derivedVariableParams                                  = &(*it);
            derivedVariableParams->inputVariableNames.at(position) = name;
            derivedVariableParams->inputVarsEnteredCount++;
            if (derivedVariableParams->inputVarsEnteredCount
                != derivedVariableParams->inputVarsNeededCount) {
              continue;
            }
          }
          std::vector<std::vector<std::string>> mergeMethodInput
            = loadComponentVariableData(*derivedVariableParams);

          Variable derivedVariable;
          if (derivedVariableParams->mergeMethod
              == ioda::detail::DataLayoutPolicy::MergeMethod::Concat) {
            std::vector<std::string> derivedVector = concatenateStringVectors(mergeMethodInput);

            const Variable firstInputVariable = this->open(
                  derivedVariableParams->inputVariableNames.front());
            // Retrieval of creation attributes and dimensions seems not to be implemented yet
            const VariableCreationParameters creationParams =
                firstInputVariable.getCreationParameters(false /*doAtts?*/, false /*doDims?*/);

            if (!dimScales) {
              // Identify all existing dimension scales
              dimScales = std::make_unique<std::list<Named_Variable>>(
                                                      identifyDimensionScales(*this, variableList));
            }

            const std::vector<std::vector<Named_Variable>> inputDimScales =
                firstInputVariable.getDimensionScaleMappings(*dimScales);

            std::vector<Variable> derivedDimScales;
            if (inputDimScales.at(0).size() == 1)
              derivedVariable = this->createWithScales<std::string>(
                    derivedVariableParams->outputName, {inputDimScales[0][0].var}, creationParams);
            else
              derivedVariable = this->create<std::string>(
                    derivedVariableParams->outputName,
                    {gsl::narrow<ioda::Dimensions_t>(derivedVector.size())},
                    {},  // max dimension
                    creationParams);

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
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda.", ioda_Here()));
  }
}

void Has_Variables_Base::convertVariableUnits(std::ostream& out) {
  try {
    if (layout_->name() != std::string("ObsGroup ODB v1")) return;
    std::vector<std::string> variableList = list();
    for (auto const& name : variableList) {
      const std::string destinationName = layout_->doMap(name);
      if (!layout_->isMapped(destinationName)) continue;
      // NOTE(Ryan): C++17 will supersede this with std::optional.
      // Check for unit. If found, unit.first == true, and unit.second is the unit.
      auto unit = layout_->getUnit(destinationName);
      if (unit.first == true) {
        Variable variableToConvert = this->open(destinationName);
        try {
          std::vector<double> outputData = variableToConvert.readAsVector<double>();
          convertColumn(unit.second, outputData);
          variableToConvert.write(outputData);
          variableToConvert.atts.add<std::string>("units", getSIUnit(unit.second));
        } catch (std::invalid_argument) {
          out << "The unit specified in ODB mapping file '" << unit.second
              << "' does not have a unit conversion defined in"
              << " UnitConversions.h, and the variable will be stored in"
              << " its original form.\n";
          variableToConvert.atts.add<std::string>("units", unit.second);
        }
      }
    }
  } catch (...) {
    std::throw_with_nested(Exception("An exception occurred inside ioda.", ioda_Here()));
  }
}

ComplementaryVariableCreationParameters Has_Variables_Base::createDerivedVariableParameters(
    const std::string &inputName, const std::string &outputName, size_t position) {
  try {
    ComplementaryVariableCreationParameters newDerivedVariable(outputName);
    const std::string destName              = layout_->doMap(inputName);
    newDerivedVariable.mergeMethod          = layout_->getMergeMethod(destName);
    newDerivedVariable.inputVarsNeededCount = layout_->getInputsNeeded(destName);
    // Populates a vector with 1 empty entry for every vector that must be entered
    std::vector<std::string> vec(newDerivedVariable.inputVarsNeededCount, "");
    newDerivedVariable.inputVariableNames              = std::move(vec);
    newDerivedVariable.inputVariableNames.at(position) = inputName;
    newDerivedVariable.inputVarsEnteredCount           = 1;
    return newDerivedVariable;
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda.", ioda_Here())
      .add("inputName", inputName).add("outputName", outputName).add("position", position));
  }
}

std::vector<std::vector<std::string> > Has_Variables_Base::loadComponentVariableData(
    const ComplementaryVariableCreationParameters &derivedVariableParams)
{
  try {
    std::vector<std::vector<std::string>> mergeMethodInput;
    for (size_t inputVariableIdx = 0;
         inputVariableIdx != derivedVariableParams.inputVarsEnteredCount; inputVariableIdx++) {
      Variable inputVariable
        = this->open(derivedVariableParams.inputVariableNames[inputVariableIdx]);
      mergeMethodInput.push_back(inputVariable.readAsVector<std::string>());
    }
    return mergeMethodInput;
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda.", ioda_Here()));
  }
}

// This is a one-level search. For searching contents of an ObsGroup, you need to
// list the Variables in each child group.
std::vector<std::string> Has_Variables_Base::list() const {
  try {
    if (backend_ == nullptr)
      throw Exception("Missing backend or unimplemented backend function.", ioda_Here());
    return backend_->list();
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while listing one-level child variables of a group.",
      ioda_Here()));
  }
}

/// @todo Extend collective variable creation interface to Python.
Variable Has_Variables_Base::_create_py(const std::string& name, BasicTypes dataType,
                             const std::vector<Dimensions_t>& cur_dimensions,
                             const std::vector<Dimensions_t>& max_dimensions,
                             const std::vector<Variable>& dimension_scales,
                             const VariableCreationParameters& params
                             ) {
  try {
    Type typ = Type(dataType, getTypeProvider());
    if (dimension_scales.size()) {
      std::vector<Dimensions_t> c_d, m_d, chunking_hints;

      for (size_t i = 0; i < dimension_scales.size(); ++i) {
        const auto& varDims = dimension_scales[i];
        const auto& d       = varDims.getDimensions();
        c_d.push_back(d.dimsCur[0]);
        m_d.push_back(d.dimsMax[0]);
        if (varDims.atts.exists("suggested_chunk_dim"))
          chunking_hints.push_back(varDims.atts.read<Dimensions_t>("suggested_chunk_dim"));
        else
          chunking_hints.push_back(-1);
      }

      VariableCreationParameters params2 = params;
      params2.chunk                      = true;
      if (!params2.chunks.size()) params2.chunks = chunking_hints;
      auto fvp = getFillValuePolicy();
      _py_fvp_helper(dataType, fvp, params2);

      // TODO(ryan): Extend collective variable creation interface to Python.
      auto var = create(name, typ, c_d, m_d, params2);
      var.setDimScale(dimension_scales);
      return var;
    } else
      return create(name, typ, cur_dimensions, max_dimensions, params);
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda.", ioda_Here()));
  }
}

void Has_Variables_Base::_py_fvp_helper(BasicTypes dataType, FillValuePolicy& fvp,
                                        VariableCreationParameters& params) {
  try {
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
              {BasicTypes::ushort_, applyFillValuePolicy<unsigned short int>},
              {BasicTypes::datetime_, applyFillValuePolicy<int64_t>},
              {BasicTypes::duration_, applyFillValuePolicy<int64_t>}
              };
    if (fvp_map.count(dataType))
      fvp_map.at(dataType)(fvp, params.fillValue_);
    else
      throw Exception("Unimplemented map entry.", ioda_Here());
  } catch (...) {
    std::throw_with_nested(Exception("An exception occurred inside ioda.", ioda_Here()));
  }
}

void Has_Variables_Base::attachDimensionScales(
  const std::vector<std::pair<Variable, std::vector<Variable>>>& mapping) {
  try {
    if (backend_ == nullptr)
      throw Exception("Missing backend or unimplemented backend function.", ioda_Here());
    backend_->attachDimensionScales(mapping);
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while attaching dimension scales.", ioda_Here()));
  }
}

void Has_Variables_Backend::attachDimensionScales(
  const std::vector<std::pair<Variable, std::vector<Variable>>>& mapping) {
  try {
    for (auto& m : mapping) {
      // The Variable{} is because the function params are a const vector<pair<...>>,
      // which implies that m.first.var would also be const. It really isn't, but C++'s const
      // implementation is a bit limited with respect to resource handles (in this case, Variables).
      Variable(m.first).setDimScale(m.second);
    }
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while attaching dimension scales.", ioda_Here()));
  }
}

Variable Has_Variables_Base::create(const std::string& name, const Type& in_memory_dataType,
                                    const std::vector<Dimensions_t>& dimensions,
                                    const std::vector<Dimensions_t>& max_dimensions,
                                    const VariableCreationParameters& params) {
  try {
    if (backend_ == nullptr)
      throw Exception("Missing backend or unimplemented backend function.", ioda_Here());
    if (layout_ == nullptr)
      throw Exception("Missing layout.", ioda_Here());

    std::vector<Dimensions_t> fixed_max_dimensions
      = (max_dimensions.size()) ? max_dimensions : dimensions;
    std::cerr << "check create\n";
    auto newVar = backend_->create(layout_->doMap(name), in_memory_dataType, dimensions,
                                   fixed_max_dimensions, params);
    std::cerr << "check 2\n";
    params.applyImmediatelyAfterVariableCreation(newVar);
    if (layout_->name() == std::string("ObsGroup ODB v1") && !(layout_->isMapped(name) ||
                                                               layout_->isComplementary(name) ||
                                                               layout_->isMapOutput(name))) {
      std::string eMessage = "The following variable was not remapped in the YAML file: '" + name +
        "'. Ensure that the fundamental dimensions are declared in 'generate'.";
      throw Exception(eMessage.c_str());
    }
    return newVar;
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while creating a variable.", ioda_Here())
      .add("name", name));
  }
}

void Has_Variables_Base::createWithScales(const NewVariables_t& newvars) {
  try {
    if (backend_ == nullptr)
      throw Exception("Missing backend or unimplemented backend function.", ioda_Here());
    
    using std::pair;
    using std::vector;
    vector<pair<Variable, vector<Variable>>> scaleMappings;
    scaleMappings.reserve(newvars.size());

    for (const auto& newvar : newvars) {
      //Type in_memory_dataType = Types::GetType<DataType>(getTypeProvider());
      Type t = (newvar->dataTypeKnown_.isValid())
                 ? newvar->dataTypeKnown_
                 : this->getTypeProvider()->makeFundamentalType(newvar->dataType_);
      std::vector<Dimensions_t> dimensions, max_dimensions, chunking_hints;
      for (size_t i = 0; i < newvar->scales_.size(); ++i) {
        const auto& varDims = newvar->scales_[i];
        const auto& d       = varDims.getDimensions();
        dimensions.push_back(d.dimsCur[0]);
        max_dimensions.push_back(d.dimsMax[0]);
        if (varDims.atts.exists("suggested_chunk_dim"))
          chunking_hints.push_back(varDims.atts.read<Dimensions_t>("suggested_chunk_dim"));
        else
          chunking_hints.push_back(-1);
      }

      // Make a copy and set chunk properties and fill value if not already set.
      // The overall use of chunking is set in params, in the .chunk bool.
      // TODO(Ryan): Differentiate the two with more distinct names.
      VariableCreationParameters params2 = newvar->vcp_;

      //auto fvp = getFillValuePolicy();
      //_py_fvp_helper(dataType, fvp, params2);
      // TODO(Ryan): Fix the fill value issue.
      //FillValuePolicies::applyFillValuePolicy<DataType>(getFillValuePolicy(), params2.fillValue_);

      params2.chunk = true;
      if (!params2.chunks.size()) params2.chunks = chunking_hints;

      auto var = create(newvar->name_, t, dimensions, max_dimensions, params2);
      using std::make_pair;
      scaleMappings.push_back(make_pair(var, newvar->scales_));
    }

    attachDimensionScales(scaleMappings);
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while creating variable(s).", ioda_Here()));
  }
}

}  // namespace detail

Has_Variables::~Has_Variables() = default;
Has_Variables::Has_Variables() : Has_Variables_Base(nullptr) {}
Has_Variables::Has_Variables(std::shared_ptr<detail::Has_Variables_Backend> b,
                             std::shared_ptr<const detail::DataLayoutPolicy> pol)
    : Has_Variables_Base(b, pol) {}

VariableCreationParameters::VariableCreationParameters() : _py_setFillValue{this} {}
VariableCreationParameters::VariableCreationParameters(const VariableCreationParameters& r)
    : fillValue_{r.fillValue_},
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
  try {
    atts.apply(h.atts);

    return h;
  } catch (...) {
    std::throw_with_nested(Exception(
      "An exception occurred inside ioda while adding attributes to an object.", ioda_Here()));
  }

}
}  // namespace ioda
