/*
* (C) Copyright 2023 NOAA/NWS/NCEP/EMC
*
* This software is licensed under the terms of the Apache Licence Version 2.0
* which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
*/

#pragma once

/*! \defgroup ioda_cxx_engines_pub_Bufr Bufr Engine
 * \brief Bufr Engine
 * \ingroup ioda_cxx_engines_pub
 *
 * @{
 * \file Bufr.h
 * \brief Bufr engine
 */

#include <map>
#include <string>

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "../defs.h"
#include "ObsStore.h"
#include "../Group.h"
#include "ioda/Engines/ReaderBase.h"

namespace ioda {
namespace Engines {
namespace Script {

  namespace py = pybind11;

  template<typename T, typename U = void>
  struct is_map : std::false_type {};

  template<typename T>
  struct is_map<T, std::void_t<typename T::key_type,
                               typename T::mapped_type>> : std::true_type {};

  /// \brief Base class for configuration arguments from the IODA configuration.
  /// \ingroup ioda_cxx_engines_pub_Script
  class ConfArgBase
  {
   public:
    explicit ConfArgBase(const std::string& name) : name_(name) {}

    virtual ~ConfArgBase() = default;

    /// \brief Get the Python object representation of the argument.
    virtual py::object getPyObject() const = 0;

    /// \brief Get the name of the argument.
    std::string name() const { return name_; }

   private:
    std::string name_;
  };

  /// \brief Template class for configuration arguments. Nested arguments are used as dict on the
  /// Python side.
  template<typename T>
  class ConfArg : public ConfArgBase
  {
   public:
    explicit ConfArg(const std::string& name, const T& value) : ConfArgBase(name), value_(value) {}

    py::object getPyObject() const final
    {
      // Interpreter must be running
      if (!Py_IsInitialized() || (PyGILState_Check() == 0))
      {
        throw eckit::BadValue("Trying to call py::cast but GIL is not available");
      }

      if constexpr (is_map<T>::value)
      {
        py::dict result;
        for (const auto& [key, value] : value_)
        {
          result[py::str(key)] = value->getPyObject();
        }

        return result;
      }
      else
      {
        return py::cast(value_);
      }
    }

  private:
    T value_;
  };

typedef  std::map<std::string, std::shared_ptr<ConfArgBase>> ConfArgMap;

/// \brief Encapsulate the parameters to make calling simpler.
/// \ingroup ioda_cxx_engines_pub_Script
struct Script_Parameters
{
  std::string scriptFile;
  ConfArgMap args;
};

/// \brief Import an Script file.
/// \ingroup ioda_cxx_engines_pub_Script
/// \param emptyStorageGroup is the initial (empty) group, provided
///   by another engine (ObsStore) that will be populated with the
///   Bufr data.
IODA_DL ObsGroup openFile(const Script_Parameters& params,
                          const ioda::Engines::ReaderCreationParameters& readerParams,
                          ioda::Group emptyStorageGroup = ObsStore::createRootGroup());

}  // namespace Script
}  // namespace Engines
}  // namespace ioda
