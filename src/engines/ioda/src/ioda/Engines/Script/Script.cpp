/*
* (C) Copyright 2023 NOAA/NWS/NCEP/EMC
*
* This software is licensed under the terms of the Apache Licence Version 2.0
* which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
*/

#include "ioda/Engines/Script.h"

#include <algorithm>
#include <pybind11/embed.h>
#include <pybind11/pybind11.h>

#include "ioda/Exception.h"
#include "ioda/Group.h"
#include "ioda/ObsGroup.h"
#include "oops/util/Logger.h"

namespace py = pybind11;

namespace ioda {
namespace Engines {
namespace Script {

namespace details {
  class ScriptInterpreter {
  public:
    ScriptInterpreter() : guard(std::make_shared<py::scoped_interpreter>()) {}

    ~ScriptInterpreter() {
      guard.reset();
      guard = nullptr;
    }

    static std::shared_ptr<ScriptInterpreter> instance() {
      static auto instance = std::make_shared<ScriptInterpreter>();
      return instance;
    }

    ScriptInterpreter& operator=(const ScriptInterpreter&) = delete;

  private:
    std::shared_ptr<py::scoped_interpreter> guard;
  };

  struct ScriptArg
  {
    std::string name;
    std::string type;
    std::string defaultValue;

    bool hasDefault() const
    {
      return defaultValue != "";
    }
  };
}  // namespace details

  /// \brief Get the arguments of a python function.
  /// \param func The python function.
  /// \return A vector of Arg objects.
  std::vector<details::ScriptArg> getArgs(py::function func)
  {
    py::module inspect = py::module::import("inspect");
    py::object signature = inspect.attr("signature")(func);
    py::dict parameters = signature.attr("parameters");

    std::vector<details::ScriptArg> result;
    for (auto item : parameters)
    {
      auto param = item.second;

      auto arg = details::ScriptArg();
      arg.name = py::str(param.attr("name"));

      py::object annotation = param.attr("annotation");
      if (!annotation.is(inspect.attr("_empty")))
      {
        arg.type = py::str(annotation.attr("__name__"));
      }

      py::object default_value = param.attr("default");
      if (!default_value.is(inspect.attr("_empty")))
      {
        if (arg.type == "")
        {
          arg.type = py::str(default_value.attr("__class__").attr("__name__"));
        }

        if (arg.type == "int" || arg.type == "float")
        {
          try
          {
            arg.defaultValue = std::stod(py::str(default_value));
          }
          catch (...)
          {
            throw Exception("Can't convert \"" + arg.type + "\" to acceptible type.", ioda_Here());
          }
        }

        if (arg.type == "bool")
        {
           arg.defaultValue  = py::str(default_value);
        }

        arg.defaultValue = py::str(default_value);
      }

      result.push_back(arg);
    }

    return result;
  }

  /// \brief Make kwargs to call the python function
  /// \param scriptParams The parameters to the script.
  /// \param scriptArgs The arguments of the function.
  /// \return A python dict containing the kwargs.
  py::dict makePythonKwArgs(const Script_Parameters& scriptParams,
                            const std::vector<details::ScriptArg>& scriptArgs,
                            const py::dict env)
  {
    // define lambda function to convert the args to the types defined in scriptArgs
    auto convertArg = [](const details::ScriptArg& scriptArg, const py::object& value) -> py::object
    {
      if (scriptArg.type == "int")
      {
        return py::int_(value);
      }

      if (scriptArg.type == "float")
      {
        return py::float_(value);
      }

      if (scriptArg.type == "bool")
      {
        if (py::isinstance<py::str>(value))
        {
          if (py::str(value).attr("lower")().cast<std::string>() == "false" || \
              py::str(value).cast<std::string>() == "0")
          {
            return py::bool_(false);
          }

          if (py::str(value).attr("lower")().cast<std::string>() == "true" || \
              py::str(value).cast<std::string>() =="1")
          {
            return py::bool_(true);
          }
        }

        if (py::isinstance<py::bool_>(value))
        {
          return value;  // already a boolean
        }

        if (py::isinstance<py::int_>(value))
        {
          return py::bool_(py::int_(value).cast<int>() != 0);
        }

        throw eckit::BadParameter("Invalid boolean value: " + py::str(value).cast<std::string>());
      }

      if (scriptArg.type == "string" || scriptArg.type == "str")
      {
        return py::str(value);
      }

      return value;  // assume it's a string or other type
    };

    py::dict kwargs;

    for (const auto& scriptArg : scriptArgs)
    {
      // add the env dictionary if the function uses it
      if (scriptArg.name == "env")
      {
        kwargs["env"] = env;
        continue;
      }

      if (scriptParams.args.find(scriptArg.name) != scriptParams.args.end())
      {
        auto argVal = scriptParams.args.at(scriptArg.name);
        kwargs[py::str(scriptArg.name)] = convertArg(scriptArg, argVal->getPyObject());
      }
      else if (scriptArg.hasDefault())
      {
        // Ignore if there is a default value
      }
      else
      {
        throw Exception("Missing required argument \"" + scriptArg.name + "\" from configuration.",
                        ioda_Here());
      }
    }
    return kwargs;
  }

  /// \brief Warn about unused arguments
  /// \param scriptParams The parameters to the script.
  /// \param args The arguments of the function.
  void warnAboutUnusedArgs(const Script_Parameters& scriptParams,
                           const std::vector<details::ScriptArg>& args)
  {
    for (const auto& arg : scriptParams.args)
    {
        bool found = false;
        for (const auto& funcArg : args)
        {
          if (arg.first == funcArg.name)
          {
            found = true;
            break;
          }
        }
        if (!found)
        {
          oops::Log::warning() << "Warning: Unused argument \""
                               << arg.first
                               << "\" in configuration."
                               << std::endl;
        }
    }
  }

  /// \brief Import an Script file.
  /// \param scriptParams The parameters to the script.
  /// \param emptyStorageGroup is the initial (empty) group.
  /// \return The ObsGroup object returned by the python function.
  ObsGroup openFile(const Script_Parameters& scriptParams,
                    const ioda::Engines::ReaderCreationParameters& readerParams,
                    Group emptyStorageGroup)
  {
    oops::Log::debug() << "Script called with " << scriptParams.scriptFile << std::endl;

    if (scriptParams.scriptFile.find(".py") == std::string::npos)
    {
      throw Exception("Unknown of script file type. Script file must be python (end in .py).",
                      ioda_Here());
    }

    const char* funcName = "create_obs_group";

    ObsGroup* obsGroup;
    py::object result;

    auto interp = details::ScriptInterpreter::instance();

    // Capture the state of the interpreter
    std::vector<std::string> defualtGlobals;
    for (const auto& global : py::globals())
    {
      defualtGlobals.push_back(py::str(global.first));
    }

    auto moduleName = py::str("ioda_script");
    auto scope = py::globals();
    py::module importlib_util = py::module::import("importlib.util");
    py::object spec = importlib_util.attr("spec_from_file_location")\
                        (moduleName, scriptParams.scriptFile);
    py::object module = importlib_util.attr("module_from_spec")(spec);
    py::module sys = py::module::import("sys");

    sys.attr("modules")[moduleName] = module;
    scope[moduleName] = module;
    spec.attr("loader").attr("exec_module")(module);

    auto func = py::cast<py::function>(module.attr(funcName));

    // Get the arguments of the function
    auto args = getArgs(func);

    // Warn about unused arguments
    warnAboutUnusedArgs(scriptParams, args);

    auto pyDatetime = py::module::import("datetime").attr("datetime");
    const py::dict env;

    env["start_time"] =
      pyDatetime.attr("strptime")(readerParams.timeWindow.start().toString(),
                                  py::str("%Y-%m-%dT%H:%M:%SZ"));
    env["end_time"] =
      pyDatetime.attr("strptime")(readerParams.timeWindow.end().toString(),
                                  py::str("%Y-%m-%dT%H:%M:%SZ"));
    env["comm_name"] = py::str(readerParams.comm.name());

    // Make kwargs to call the python function
    py::dict kwargs = makePythonKwArgs(scriptParams, args, env);

    try {
        // Call the python function
        result = func(**kwargs);
    } catch (const py::error_already_set& e) {
        throw Exception("Python error: " + std::string(e.what()), ioda_Here());
    }

    // Check that the python function returned an ObsGroup object
    obsGroup = py::cast<ObsGroup*>(result);
    if (obsGroup == nullptr) {
        throw Exception("Function \"create_obs_group\" did not return an ObsGroup object.",
                        ioda_Here());
    }

    // Return the interpreter back to its initial state
    for (const auto& global : scope)
    {
        std::string globalName = py::str(global.first);
        if (std::find(defualtGlobals.begin(),
                      defualtGlobals.end(),
                      globalName) == defualtGlobals.end())
        {
          py::globals().attr("pop")(globalName);
        }
    }

    return *obsGroup;
  }

}  // namespace Script
}  // namespace Engines
}  // namespace ioda
