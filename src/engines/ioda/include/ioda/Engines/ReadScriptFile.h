/*
* (C) Copyright 2023 NOAA/NWS/NCEP/EMC
*
* This software is licensed under the terms of the Apache Licence Version 2.0
* which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
*/

#pragma once

#include <variant>
#include <string>
#include <vector>
#include <map>
#include <memory>



#include "ioda/Engines/ReaderBase.h"
#include "ioda/Engines/Script.h"
#include "oops/util/parameters/ObjectJsonSchema.h"
#include "oops/util/parameters/ParameterTraits.h"
#include "oops/util/CompositePath.h"

namespace ioda {
namespace Engines {

//----------------------------------------------------------------------------------------
// ReadScriptFile
//----------------------------------------------------------------------------------------

class ArgsParameter : public oops::ParameterBase
{
 public:
  explicit ArgsParameter(const char *name, oops::Parameters *parent)
  : ParameterBase(parent), name_(name)
  {}

  void deserialize(util::CompositePath& path,
                    const eckit::Configuration& config) final
  {
    // copy the configuration for when we re-serialize (why?).
    config_ = eckit::LocalConfiguration(config.getSubConfiguration(name_));
    args_ = parseConf(config, name_);
  }

  void serialize(eckit::LocalConfiguration& config) const final
  {
    if (args_.size())
    {
      config.set(name_, config_);
    }
  }

  oops::ObjectJsonSchema jsonSchema() const final
  {
    return oops::ObjectJsonSchema({{"args", {{"type", "\"object\""}}}});
  }

  Script::ConfArgMap values() const
  {
    return args_;
  }

 private:
  std::string name_;
  Script::ConfArgMap args_;
  eckit::LocalConfiguration config_;

  Script::ConfArgMap parseConf(const eckit::Configuration& config, const std::string& name)
  {
    Script::ConfArgMap args;
    auto argConf = config.getSubConfiguration(name);

    for (auto argName : argConf.keys())
    {
      auto cleanedArgName = argName;
      std::replace(cleanedArgName.begin(), cleanedArgName.end(), ' ', '_');

      if (argConf.isFloatingPoint(argName))
      {
        args[cleanedArgName] = std::make_shared<Script::ConfArg<float>>(
          argName, argConf.getFloat(argName));
      }
      else if (argConf.isIntegral(argName))
      {
        args[cleanedArgName] = std::make_shared<Script::ConfArg<int>>(
          argName, argConf.getInt(argName));
      }
      else if (argConf.isString(argName))
      {
        args[cleanedArgName] = std::make_shared<Script::ConfArg<std::string>>(
          argName, argConf.getString(argName));
      }
      else if (argConf.isBoolean(argName))
      {
        args[cleanedArgName] = std::make_shared<Script::ConfArg<bool>>(
          argName, argConf.getBool(argName));
      }
      else if (argConf.isList(argName))
      {
        if (argConf.isStringList(argName))
        {
          args[cleanedArgName] = std::make_shared<Script::ConfArg<std::vector<std::string>>>(
            argName, argConf.getStringVector(argName));
        }
        else if (argConf.isFloatingPointList(argName))
        {
          args[cleanedArgName] = std::make_shared<Script::ConfArg<std::vector<float>>>(argName,
            argConf.getFloatVector(argName));
        }
        else if (argConf.isIntegralList(argName))
        {
          args[cleanedArgName] = std::make_shared<Script::ConfArg<std::vector<int>>>(argName,
            argConf.getIntVector(argName));
        }
        else if (argConf.isBooleanList(argName))
        {
          args[cleanedArgName] = std::make_shared<Script::ConfArg<std::vector<int>>>(argName,
            argConf.getIntVector(argName));
        }
        else if (argConf.isSubConfigurationList(argName))
        {
          std::vector<Script::ConfArgMap> argMaps;
          for (const auto& subConf : argConf.getSubConfigurations(argName))
          {
            auto argMap = parseConf(subConf, subConf.keys()[0]);
            argMaps.push_back(argMap);
          }

          args[cleanedArgName] = std::make_shared<Script::ConfArg<std::vector<Script::ConfArgMap>>>(argName, argMaps);
        }
      }
      else if (argConf.isSubConfiguration(argName))
      {
        auto argMap = parseConf(argConf, argName);
        args[cleanedArgName] = std::make_shared<Script::ConfArg<Script::ConfArgMap>>(argName, argMap);
      }
    }

    return args;
  }
};

class ReadScriptFileParameters : public ReaderParametersBase {
  OOPS_CONCRETE_PARAMETERS(ReadScriptFileParameters, ReaderParametersBase)

public:
  /// \brief Path to input file
  ArgsParameter args{"args", this};

  /// \brief Path to odc query specs
  oops::RequiredParameter<std::string> scriptFile{"script file", this};

  bool isFileBackend() const override { return false; }

  std::string getFileName() const override { return std::string(""); }
};

// Classes

class ReadScriptFile: public ReaderBase {
public:
  typedef ReadScriptFileParameters Parameters_;

  // Constructor via parameters
  ReadScriptFile(const Parameters_& params, const ReaderCreationParameters & createParams);

  void print(std::ostream & os) const final;

  std::string fileName() const final;

private:
  std::string fileName_;
};

}  // namespace Engines
}  // namespace ioda
