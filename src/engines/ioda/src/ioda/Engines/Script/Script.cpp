/*
* (C) Copyright 2023 NOAA/NWS/NCEP/EMC
*
* This software is licensed under the terms of the Apache Licence Version 2.0
* which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
*/

#include <pybind11/pybind11.h>
#include <pybind11/embed.h>

#include "ioda/Exception.h"
#include "ioda/ObsGroup.h"
#include "ioda/Group.h"
#include "ioda/Engines/Script.h"
#include "oops/util/Logger.h"
#include "eckit/config/YAMLConfiguration.h"

namespace py = pybind11;

namespace ioda {
namespace Engines {
namespace Script {

  struct Arg
  {
        std::string name;
        std::string type;
        std::string defaultValue;

        bool hasDefault() const
        {
          return defaultValue != "";
        }
  };

  /// \brief Get the arguments of a python function.
  /// \param func The python function.
  /// \return A vector of Arg objects.
  std::vector<Arg> getArgs(py::function func)
  {
    py::module inspect = py::module::import("inspect");
    py::object signature = inspect.attr("signature")(func);
    py::dict parameters = signature.attr("parameters");

    std::vector<Arg> result;
    for (auto item : parameters)
    {
      auto param = item.second;

      auto arg = Arg();
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
//          arg.defaultValue = (py::str(default_value) == "True"));
        }
        else
        {
          arg.defaultValue = py::str(default_value);
        }
      }

      result.push_back(arg);
    }

    return result;
  }

  /// \brief Make kwargs to call the python function
  /// \param scriptParams The parameters to the script.
  /// \param args The arguments of the function.
  /// \return A python dict containing the kwargs.
  py::dict makePythonKwArgs(const Script_Parameters& scriptParams, const std::vector<Arg>& args)
  {
    py::dict kwargs;

    for (const auto& arg : args)
    {
      if (scriptParams.args.find(arg.name) != scriptParams.args.end())
      {
        Script_Parameters::ArgType argVal = scriptParams.args.at(arg.name);

        if (arg.type == "int")
        {
          std::visit([&kwargs, arg](const auto& paramArg)
          {
            using T = std::decay_t<decltype(paramArg)>;
            if constexpr (std::is_same_v<T, int>)
             kwargs[arg.name.c_str()] = paramArg;
            else if constexpr (std::is_same_v<T, double>)
             kwargs[arg.name.c_str()] = static_cast<int>(paramArg);
            else
             throw Exception("Can't convert \"" + arg.type + "\" to acceptible type.", ioda_Here());

          }, argVal);
        }
        else if (arg.type == "float")
        {
          std::visit([&kwargs, arg](const auto& paramArg)
          {
            using T = std::decay_t<decltype(paramArg)>;
            if constexpr (std::is_same_v<T, double>)
             kwargs[arg.name.c_str()] = paramArg;
            else if constexpr (std::is_same_v<T, int>)
             kwargs[arg.name.c_str()] = paramArg;
            else
             throw Exception("Can't convert \"" + arg.type + "\" to acceptible type.", ioda_Here());

          }, argVal);
        }
        else if (arg.type == "bool")
        {
          std::visit([&kwargs, arg](const auto& paramArg)
          {
           using T = std::decay_t<decltype(paramArg)>;
           if constexpr (std::is_same_v<T, std::string>)
             if (paramArg == "true" || paramArg == "True")
             {
               kwargs[arg.name.c_str()] = true;
             }
             else if (paramArg == "false" || paramArg == "False")
             {
               kwargs[arg.name.c_str()] = false;
             }
             else
             {
               throw Exception("Can't convert \"" + paramArg + "\" to bool.", ioda_Here());
             }
           else if constexpr (std::is_same_v<T, int>)
             kwargs[arg.name.c_str()] = (paramArg != 0);
           else
             throw Exception("Can't convert \"" + arg.type + "\" to acceptible type.", ioda_Here());

          }, argVal);
        }
        else
        {
          std::visit([&kwargs, arg](const auto& paramArg)
          {
            using T = std::decay_t<decltype(paramArg)>;
            if constexpr (std::is_same_v<T, std::string>)
              kwargs[arg.name.c_str()] = paramArg;
            else if constexpr (std::is_same_v<T, int>)
              kwargs[arg.name.c_str()] = paramArg;
            else if constexpr (std::is_same_v<T, double>)
              kwargs[arg.name.c_str()] = paramArg;
            else
             throw Exception("Can't convert \"" + arg.type + "\" to acceptible type.", ioda_Here());

          }, argVal);
        }
      }
      else if (arg.hasDefault())
      {
        // Ignore if there is a default value
      }
      else
      {
        throw Exception("Missing required argument \"" + arg.name + "\" from configuration.",
                        ioda_Here());
      }
    }
    return kwargs;
  }

  /// \brief Warn about unused arguments
  /// \param scriptParams The parameters to the script.
  /// \param args The arguments of the function.
  void warnAboutUnusedArgs(const Script_Parameters& scriptParams, const std::vector<Arg>& args)
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
  ObsGroup openFile(const Script_Parameters& scriptParams, Group emptyStorageGroup)
  {
    oops::Log::debug() << "Script called with " << scriptParams.scriptFile << std::endl;

    if (scriptParams.scriptFile.find(".py") == std::string::npos)
    {
      throw Exception("Unknown of script file type. Script file must be python (end in .py).",
                      ioda_Here());
    }

    const char* funcName = "create_obs_group";

    // Start the python interpreter
    py::scoped_interpreter guard{};

    // Add the script file to the python path
    py::object scope = py::globals();
    scope["__file__"] = scriptParams.scriptFile;

    try 
    {
        py::gil_scoped_acquire acquire;
        py::eval_file(scriptParams.scriptFile, scope);
    } 
    catch (const py::error_already_set& e) 
    {
        std::cerr << "Python error: " << e.what() << std::endl;
    } 
    catch (const std::exception& e) 
    {
        std::cerr << "Standard exception: " << e.what() << std::endl;
    } 
    catch (...) 
    {
        std::cerr << "Unknown exception occurred." << std::endl;
    }

    // Get a reference to the function
    auto func = py::cast<py::function>(scope[funcName]);

    // Get the arguments of the function
    auto args = getArgs(func);

    // Warn about unused arguments
    warnAboutUnusedArgs(scriptParams, args);

    // Make kwargs to call the python function
    py::dict kwargs = makePythonKwArgs(scriptParams, args);

    py::object result;
    try
    {
      // Call the python function
      result = func(**kwargs);
    }
    catch (const py::error_already_set& e)
    {
      throw Exception("Python error: " + std::string(e.what()), ioda_Here());
    }

    // Check that the python function returned an ObsGroup object
    auto obsGroup = py::cast<ObsGroup*>(result);
    if (obsGroup == nullptr)
    {
      throw Exception("Function \"create_obs_group\" did not return an ObsGroup object.",
                      ioda_Here());
    }

    return *obsGroup;
  }

}  // namespace Script
}  // namespace Engines
}  // namespace ioda
