#pragma once
/*
 * (C) Crown copyright 2026, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <cstdint>
#include <string>

namespace ioda {

namespace Engines {

/// Type of a variable that can be stored in a container wrapped by a ContainerFacade.
enum class ContainerVariableType : std::int8_t {
  Int,
  Int64,
  Float,
  Char,
  String
};

/// \brief The member of ContainerVariableType corresponding to type T.
template <typename T>
ContainerVariableType containerVariableType;

template<>
inline constexpr ContainerVariableType containerVariableType<int> =
  ContainerVariableType::Int;

template<>
inline constexpr ContainerVariableType containerVariableType<int64_t> =
  ContainerVariableType::Int64;

template<>
inline constexpr ContainerVariableType containerVariableType<float> =
  ContainerVariableType::Float;

template<>
inline constexpr ContainerVariableType containerVariableType<std::string> =
  ContainerVariableType::String;

template<>
inline constexpr ContainerVariableType containerVariableType<char> =
  ContainerVariableType::Char;

}  // namespace Engines
}  // namespace ioda
