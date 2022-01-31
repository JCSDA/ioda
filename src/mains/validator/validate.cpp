/*
 * (C) Copyright 2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! @file validate.cpp
* @brief A program to validate ioda file contents.
* 
* Call program as: ioda-validate.x yaml-file input-file
*/

#include <iomanip>
#include <ios>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "./AttributeChecks.h"
#include "./Log.h"
#include "./Params.h"
#include "eckit/config/YAMLConfiguration.h"
#include "eckit/log/Colour.h"
#include "eckit/runtime/Main.h"
#include "ioda/Engines/Factory.h"
#include "ioda/Engines/HH.h"
#include "ioda/Exception.h"
#include "ioda/Group.h"
#include "ioda/Misc/StringFuncs.h"
#include "ioda/Variables/VarUtils.h"
#include "ioda/Units.h"
#include "oops/mpi/mpi.h"
#include "oops/runs/Application.h"
#include "oops/util/LibOOPS.h"

class Validator : public eckit::Main {
  Results res_;
  ioda_validate::IODAvalidateParameters params_;

 public:
  virtual ~Validator() {}
  explicit Validator(int argc, char **argv) : eckit::Main(argc, argv) {}

  int execute() {
    using std::cout;
    using std::cerr;
    using std::endl;
    using std::exception;
    using std::string;
    using ioda::Exception;
    int ret = 0;
    try {
      if (argc() != 3) throw Exception("Usage: ioda-validate.x yaml-file input-file", ioda_Here());

      eckit::PathName yamlfilename(argv(1));
      std::string datafilename(argv(2));

      cout << "Reading YAML from " << yamlfilename << endl;

      eckit::YAMLConfiguration yaml(yamlfilename);
      params_.validateAndDeserialize(yaml);

      Log::LogContext lg(std::string("Processing data file: ").append(datafilename));
      const ioda::Group base = ioda::Engines::HH::openMemoryFile(datafilename);
      validate(base);
    } catch (const exception &e) {
      cerr << e.what() << endl;
      res_.numErrors++;
      ret = 1;
    }
    eckit::Colour::reset(cout);
    eckit::Colour::underline(cout);
    cout << "Final results:";
    eckit::Colour::reset(cout);
    eckit::Colour::red(cout);
    cout << "\n  # errors:   " << std::right << std::setw(4) << res_.numErrors;
    eckit::Colour::reset(cout);
    eckit::Colour::blue(cout);
    cout << "\n  # warnings: " << std::setw(4) << res_.numWarnings << endl;
    eckit::Colour::reset(cout);
    cout << "\nCTEST_FULL_OUTPUT\n";

    // TODO(ryan): Once the number of errors and warnings in the
    //   ufo-data and ioda-converters repositories are at a minimal
    //   level, replace with (res_.numErrors || res_.numWarnings) ? 1 : 0;
    //   or with (res_.numErrors) ? 1 : 0;
    return ret;
  }

  void validate(const ioda::Group &base) {
    using ioda::ObjectType;
    using ioda::VarUtils::VarDimMap;
    using ioda::VarUtils::Vec_Named_Variable;
    using ioda_validate::AttributeParameters;
    using ioda_validate::DimensionParameters;
    using ioda_validate::GroupParameters;
    using ioda_validate::VariableParameters;
    using ioda_validate::Severity;
    using Log::LogContext;
    using Log::log;
    using std::map;
    using std::set;
    using std::string;
    using std::vector;

    map<string, AttributeParameters> YAMLattributes;  // Used throughout the checks
    map<string, string> YAMLattributesOldNewNames;
    for (const auto &ya : params_.attributes.value())
      for (const auto &yname : ya.attname.value()) {
        YAMLattributes.insert(make_pair(yname, ya));
        if (yname != ya.attname.value()[0])
          YAMLattributesOldNewNames[yname] = ya.attname.value()[0];
      }

    // Iterate through the group tree to perform checks

    // Take the groups in the YAML schema and convert from a std::vector to a std::map<name, group>.
    map<string, GroupParameters> YAMLgroups;
    map<string, string> YAMLgroupsOldNewNames;
    for (const auto &yg : params_.groups.value())
      for (const auto &ygname : yg.grpname.value()) {
        YAMLgroups.insert(make_pair(ygname, yg));
        if (ygname != yg.grpname.value()[0])
          YAMLattributesOldNewNames[ygname] = yg.grpname.value()[0];
      }

    auto vGroupNames = base.listObjects<ObjectType::Group>(true);
    vGroupNames.push_back("/");  // Add in the root group.

    // Check that required groups exist
    {
      LogContext lg0("Verifying that all required groups exist");
      auto sGroupNames = set<string>(vGroupNames.begin(), vGroupNames.end());
      for (const auto &yg : YAMLgroups) {
        if (yg.second.required.value()) {
          if (!sGroupNames.count(yg.first)) {
            bool hasoldname = false;
            // Check for old names
            for (const auto &oldname : yg.second.grpname.value()) {
              if (sGroupNames.count(oldname)) {
                log(params_.policies.value().RequiredGroups.value(), res_)
                  << "Required group " << yg.first << " is using an older name: '" << oldname
                  << "'.\n";
                hasoldname = true;
              }
            }
            if (!hasoldname)
              log(params_.policies.value().RequiredGroups.value(), res_)
                << "Required group " << yg.first << " is missing.\n";
          }
        }
      }
    }

    // ALl other group checks
    for (const auto &gn : vGroupNames) {
      if (YAMLgroups.count(gn)) {
        LogContext lg1(std::string("Verifying group ").append(gn));
        log(Severity::Debug) << "Group '" << gn << "' is described in the YAML file.\n";
        if (YAMLgroups.at(gn).remove.value()) {
          log(params_.policies.value().GroupsKnown.value(), res_)
            << "Group " << gn << " is deprecated. " << *YAMLgroups.at(gn).remove.value() << "\n";
        }

        // Check group attributes
        const auto grp = base.open(gn);
        // As vectors
        const auto vGrpAttNames = grp.atts.list();
        const auto vYAMLreqAtts = YAMLgroups.at(gn).atts.value().required.value();
        const auto vYAMLoptAtts = YAMLgroups.at(gn).atts.value().optional.value();
        // As sets
        const set<string> sGrpAttNames(vGrpAttNames.begin(), vGrpAttNames.end());
        const set<string> sYAMLreqAtts(vYAMLreqAtts.begin(), vYAMLreqAtts.end());
        const set<string> sYAMLoptAtts(vYAMLoptAtts.begin(), vYAMLoptAtts.end());

        requiredSymbolsCheck(vYAMLreqAtts, sGrpAttNames, params_, res_);
        appropriateAttributesCheck(vGrpAttNames, sYAMLreqAtts, sYAMLoptAtts, params_, res_);
        matchingAttributesCheck(YAMLattributes, vGrpAttNames, grp.atts, params_, res_);

        // Check that each group's required variables exist (mostly for
        //  metadata. latitude, longitude, datetime)
        const auto vYAMLreqVars
          = YAMLgroups.at(gn).requiredvars.value();  // key may or may not exist
        if (vYAMLreqVars) {
          const auto vGrpVarNames = grp.vars.list();
          const set<string> sGrpVarNames(vGrpVarNames.begin(), vGrpVarNames.end());
          requiredSymbolsCheck(*vYAMLreqVars, sGrpVarNames, params_, res_);
        }

      } else {
        log(params_.policies.value().GroupsKnown.value(), res_)
          << "Group " << gn << " is not described in the YAML file.\n";
      }
    }

    // Dimension scale and variable-level checks

    // Enumerate all variables in all groups
    // Determine which are dimensions and which are regular variables.
    // Determine dimension attachments to variables.
    Vec_Named_Variable fileVars, fileDims;
    map<string, Vec_Named_Variable> dimsAttachedToVars;
    {
      VarDimMap dimsAttachedToVars_;
      ioda::Dimensions_t maxVarSize0;  // junk
      ioda::VarUtils::collectVarDimInfo(
        base, fileVars, fileDims, dimsAttachedToVars_, maxVarSize0);
      for (const auto &m : dimsAttachedToVars_) dimsAttachedToVars[m.first.name] = m.second;
    }

    // Verify dimension names
    map<string, string> sOldNewDimNames;  // Used later in variable dimensions checks
    {
      LogContext lg("Verifying dimension names");
      auto vDimParams = params_.dimensions.value();
      map<string, DimensionParameters> mDimParams;
      for (const auto &YAMLdim : vDimParams) {
        // double for loop since dimension names can have multiple values in the YAML
        // ordering is [ preferred_dim_name, other_dim_name_1, other_dim_name_2, ... ]
        const auto YAMLdimNames = YAMLdim.dimname.value();
        if (YAMLdimNames.empty()) {
          log(Severity::Error) << "YAML spec for dimension names is buggy\n";
          continue;
        }
        string preferredDimName = YAMLdimNames[0];
        for (const auto &YAMLdimName : YAMLdimNames) {
          mDimParams[YAMLdimName] = YAMLdim;
          if (YAMLdimName != preferredDimName) sOldNewDimNames[YAMLdimName] = preferredDimName;
        }

        // Also, check that all required dimensions exist
        if (YAMLdim.required.value()) {
          bool found = false;
          for (const auto &fileDim : fileDims) {
            for (const auto &YAMLdimName : YAMLdimNames)
              if (fileDim.name == YAMLdimName) {
                found = true;
                log(Severity::Debug)
                  << "Required dimension '" << fileDim.name << "' is found in the file.\n";
              }
          }
          if (!found)
            log(params_.policies.value().RequiredDimensions.value(), res_)
              << "Dimension " << YAMLdimNames[0]
              << " (and all of this dimension's alternate names) is missing from the file.\n";
        }
      }

      for (const auto &fileDim : fileDims) {
        if (mDimParams.count(fileDim.name)) {
          log(Severity::Debug) << "Dimension " << fileDim.name << " is known.\n";

          // Old dimension name check
          if (sOldNewDimNames.count(fileDim.name)) {
            log(params_.policies.value().DimensionsUseNewName, res_)
              << "Dimension '" << fileDim.name
              << "' is from an old standard. "
                 "Prefer using the new name '"
              << sOldNewDimNames.at(fileDim.name) << "'.\n";
          }

          // Check the dimension's dimensionality.
          auto dims = fileDim.var.getDimensions();
          if (dims.dimensionality > 1)
            log(params_.policies.value().GeneralDimensionsChecks, res_)
              << "Dimension '" << fileDim.name << "' has incorrect dimensionality.\n";

          // TODO(ryan): dimension type check needs another IODA PR
          log(Severity::Trace, res_) << "TODO: Implement dimension type check.\n";
        } else {
          log(params_.policies.value().DimensionsKnown, res_)
            << "Dimension " << fileDim.name << " is not described in the YAML file.\n";
        }
      }
    }

    // Verify variable information
    {
      LogContext lg("Verifying variable information");
      auto varParamsDefault = params_.vardefaults.value();
      auto vVarParams       = params_.variables.value();
      map<string, VariableParameters> mVarParams;
      map<string, string> mOldNewVarNames;
      for (const auto &v : vVarParams) {
        VariableParameters resultingVarParams = v;
        // Replace any of the missing parameters with parameters from the defaults.
        if (!v.base.atts.value()) resultingVarParams.base.atts = varParamsDefault.atts;
        if (!v.base.canBeMetadata.value())
          resultingVarParams.base.canBeMetadata = varParamsDefault.canBeMetadata;
        if (!v.base.dimNames.value()) resultingVarParams.base.dimNames = varParamsDefault.dimNames;
        if (!v.base.type.value()) resultingVarParams.base.type = varParamsDefault.type;

        // Variable name can be either a single string or a vector of strings.
        // Unfortunately, the Parameters parser is buggy here.
        // try {
        // Vector of strings case
        vector<string> names = v.varname.value().as<vector<string>>();
        for (const auto &name : names) {
          mVarParams[name] = resultingVarParams;
          if (name != names[0]) mOldNewVarNames[name] = names[0];
        }
        /*} catch (...) {
                // Single string case
                string name = v.varname.value().as<string>();
                mVarParams[name] = resultingVarParams;
            }*/
      }

      for (const auto &v : fileVars) {
        // The variable name is reported as group/name. Split this up into
        //  group and name components.
        const vector<string> splitName = ioda::splitPaths(v.name);
        if (splitName.size() != 2) {
          log(Severity::Error, res_)
            << "Skipping processing of '" << v.name << "'. Unsure how to parse this name.\n";
          continue;
        }
        const string group = splitName[0];
        const string name  = splitName[1];

        {
          LogContext lg(string("Variable ").append(v.name));

          // Is this name known to the conventions?
          if (mVarParams.count(name) == 0) {
            log(params_.policies.value().KnownVariableNames, res_)
              << "Variable '" << v.name << "' is not listed in the YAML conventions file.\n";
            continue;
          }

          // Old vs new name check
          if (mOldNewVarNames.count(name)) {
            log(params_.policies.value().VariableUseNewName, res_)
              << "Variable '" << v.name << "' uses a superseded name. Replace with '"
              << mOldNewVarNames.at(name) << "'\n";
          }

          // Get params
          auto varparams = mVarParams.at(name);

          // Variable should be removed check
          if (varparams.remove.value()) {
            log(params_.policies.value().VariableUseNewName, res_)
              << "Variable '" << v.name << "' is deprecated and should be removed.\n";
            continue;
          }

          // Apply group-specific overrides (type)

          if (YAMLgroups.count(group)) {
            auto YAMLgroup = YAMLgroups.at(group);

            // Check that a regular variable is allowed within this group
            if (YAMLgroup.regularVariablesAllowed.value() == false)
              log(params_.policies.value().GroupAllowsVariables, res_)
                << "Variable '" << v.name << "' is in a group '" << group
                << "' that disallows regular (non-dimension-scale) variables.\n";

            // Override type
            if (YAMLgroup.type.value()) varparams.base.type = YAMLgroup.type;
          } else {
            log(params_.policies.value().GroupsKnown, res_)
              << "Variable '" << v.name << "' is in unknown group '" << group << "'.\n";
          }

          // Can this variable be in the Metadata group?
          if (group == "MetaData" && (varparams.base.canBeMetadata.value() == false))
            log(params_.policies.value().VariableCanBeMetadata, res_)
              << "Variable '" << v.name << "' should not be in MetaData.\n";

          // Recommended dimension scales check
          if (varparams.base.dimNames.value()) {
            const vector<string> recommendedDimensions = *(varparams.base.dimNames.value());
            auto varDimensionsCur_
              = dimsAttachedToVars.at(v.name);  // Dimensions attached, but perhaps with old names
            vector<string> varDimensionsCur;
            for (const auto &d : varDimensionsCur_)
              varDimensionsCur.emplace_back(
                (mOldNewVarNames.count(d.name)) ? mOldNewVarNames.at(d.name) : d.name);

            // TODO(ryan) for future: some variables like scanAngle can have different dimensions
            //  depending on instrument. The YAML spec should be adapted in these cases
            //  to allow a boost::variant of either a vector<string>
            //  or a vector<vector<string>>.

            // Are the dimensions the same size?
            if ((recommendedDimensions.size() == varDimensionsCur.size())
                && (static_cast<size_t>(v.var.getDimensions().dimensionality))
                     == varDimensionsCur.size()) {
              for (size_t i = 0; i < recommendedDimensions.size(); ++i) {
                string curDim = varDimensionsCur[i];
                if (sOldNewDimNames.count(curDim))
                  curDim = sOldNewDimNames.at(curDim);  // Old to new name.
                if (recommendedDimensions[i] != curDim) {
                  log(params_.policies.value().VariableDimensionCheck, res_)
                    << "Variable '" << v.name
                    << "' does not have recommended dimensions along "
                       "index "
                    << i << ". Recommended [i]: " << recommendedDimensions[i]
                    << ", and actual [i]: " << varDimensionsCur[i] << ".\n";
                }
              }
            } else {
              log(params_.policies.value().VariableDimensionCheck, res_)
                << "Variable '" << v.name
                << "' has a mismatch in "
                   "attached dimensions vs those specified in the YAML. YAML dimensionality is "
                << recommendedDimensions.size() << ", variable dimensionality is "
                << v.var.getDimensions().dimensionality
                << ", and the attached dimensions along the variable have dimensionality "
                << varDimensionsCur.size() << ".\n";
            }
          }

          // Do dimension lengths match those of the associated dimension scales?
          {
            const auto &dimscales = dimsAttachedToVars.at(v.name);
            const auto dims       = v.var.getDimensions().dimsCur;
            for (size_t i = 0; i < dims.size(); ++i) {
              if (static_cast<size_t>(dims[i])
                  != static_cast<size_t>(dimscales[i].var.getDimensions().numElements))
                log(params_.policies.value().VariableDimensionCheck, res_)
                  << "Variable '" << v.name << "' dimension " << i
                  << " has a length that differs from its attached dimension scale, '"
                  << dimscales[i].name << "', which has a length of "
                  << dimscales[i].var.getDimensions().numElements << ".\n";
            }
          }

          // Type check
          log(Severity::Trace, res_) << "TODO: Implement type check.\n";

          // Attributes checks (required and optional attributes;
          //  attribute dimension and type checks)
          {
            // LogContext lg("Checking variable attributes");
            if (varparams.base.atts.value()) {
              // Check for required and otherwise recognized attributes
              auto req_        = varparams.base.atts.value()->base.required.value();
              auto opt_        = varparams.base.atts.value()->base.optional.value();
              auto reqNotEnum_ = varparams.base.atts.value()->requiredNotEnum.value();
              set<string> req(req_.begin(), req_.end());
              set<string> opt(opt_.begin(), opt_.end());

              if (varparams.base.type.value()) {
                // The type parameter can either be an enum or a full type description
                ioda_validate::Type typ = ioda_validate::Type::Unspecified;
                // try {
                typ = varparams.base.type.value()->as<ioda_validate::Type>();
                /*} catch (...) {
                                typ = *(varparams.base.type.value()->as<TypeParameters>().type.value());
                            }*/
                if (typ != ioda_validate::Type::Enum)
                  for (const auto &r : reqNotEnum_) req.insert(r);
              }

              auto attNames = v.var.atts.list();

              appropriateAttributesCheck(attNames, req, opt, params_, res_);
            }

            matchingAttributesCheck(YAMLattributes, v.var.atts.list(), v.var.atts, params_, res_);
          }

          // Units (check that units are set if needed, check compatible units, check exact units)
          bool unitsRequired = false;
          bool unitsDisabled = false;
          if (varparams.forceunits.value()) {
            unitsRequired = *varparams.forceunits.value();
            if (!unitsRequired) unitsDisabled = true;  // Force Units is set to false
          } else {
            if (varparams.base.type.value()) {
              // The type parameter can either be an enum or a full type description
              ioda_validate::Type typ = ioda_validate::Type::Unspecified;
              // try {
              typ = varparams.base.type.value()->as<ioda_validate::Type>();
              /*} catch (...) {
                            typ = *(varparams.base.type.value()->as<TypeParameters>().type.value());
                        }*/
              if (typ != ioda_validate::Type::Enum && typ != ioda_validate::Type::StringVLen
                  && typ != ioda_validate::Type::StringFixedLen)
                unitsRequired = true;
            }
          }

          const auto varparamsAtts = varparams.attributes.value();
          bool checkUnits          = (unitsDisabled) ? false
                                                     : v.var.atts.exists("units")
                                                || varparamsAtts.count("units") || unitsRequired;

          if (unitsDisabled) {
            if (v.var.atts.exists("units"))
              log(params_.policies.value().VariableHasConvertibleUnits, res_)
                << "File variable '" << v.name << "' has units of '"
                << v.var.atts.read<string>("units")
                << "', but the YAML spec prohibits units for this variable.\n";
          }
          if (checkUnits) {
            // LogContext lg("Checking units");
            string sVarUnits, sYAMLunits;
            if (v.var.atts.exists("units"))
              sVarUnits = v.var.atts.read<string>("units");
            else
              log(params_.policies.value().VariableHasValidUnits, res_)
                << "Variable '" << v.name
                << "' needs units, but the 'units' attribute does not exist in the file.\n";
            if (varparamsAtts.count("units"))
              sYAMLunits = varparamsAtts.at("units");
            else
              log(params_.policies.value().VariableHasValidUnits, res_)
                << "Variable '" << v.name
                << "' needs units, but the 'units' attribute does not exist in the YAML.\n";

            if (sVarUnits.size() && sYAMLunits.size()) {
              const auto varUnits = ioda::udunits::Units(sVarUnits);
              const auto YAMLUnits = ioda::udunits::Units(sYAMLunits);

              // Check for valid units
              if (!varUnits.isValid())
                log(params_.policies.value().VariableHasConvertibleUnits, res_)
                  << "File variable '" << v.name << "' has units of '" << sVarUnits
                  << "', which are invalid.\n";
              if (!YAMLUnits.isValid())
                log(params_.policies.value().VariableHasConvertibleUnits, res_)
                  << "The YAML spec for variable '" << v.name << "' has units of '" << sYAMLunits
                  << "', which are invalid.\n";

              if (varUnits.isValid() && YAMLUnits.isValid()) {
                // Check for convertible units
                if (!varUnits.isConvertibleWith(YAMLUnits))
                  log(params_.policies.value().VariableHasConvertibleUnits, res_)
                    << "Variable '" << v.name << "' has units of '" << varUnits
                    << "', which are not convertible to the YAML-specified units of '" << sYAMLunits
                    << "'.\n";

                // Check for exact units
                if (!(varUnits == YAMLUnits) && varparams.checkExactUnits.value())
                  log(params_.policies.value().VariableHasExactUnits, res_)
                    << "Variable '" << v.name << "' has units of '" << varUnits
                    << "'. The YAML-specified units are '" << sYAMLunits
                    << "'."
                       " Although convertible, these units are not equivalent.\n";
              }
            }
          }

          // These are not needed now, but they may be useful in the future
          // Expected Variable range (check data range if set)
          log(Severity::Trace, res_) << "TODO: Implement variable range (ExpectedRange) check.\n";

          // Is a fill value set appropriately (both for HDF5 and NetCDF4)?
          log(Severity::Trace, res_) << "TODO: Implement fill value check.\n";

          // Is chunking enabled?
          log(Severity::Trace, res_) << "TODO: Implement chunking check.\n";

          // Are the chunk sizes sensible?
          log(Severity::Trace, res_) << "TODO: Implement chunk size check.\n";

          // Is compression enabled?
          log(Severity::Trace, res_) << "TODO: Implement compression check.\n";
        }
      }
    }
  }
};

int main(int argc, char **argv) {
  Validator run(argc, argv);
  return run.execute();
}
