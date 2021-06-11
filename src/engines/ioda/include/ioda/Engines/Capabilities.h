#pragma once
/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_cxx_engines_pub
 *
 * @{
 * \file Capabilities.h
 * \brief Structs that describe backend capabilities.
 */
#include <string>

#include "../defs.h"

namespace ioda {
class Group;

namespace Engines {
/// \brief A tri-bool type that indicates whether a feature is supported,
///   ignored if used, or if the engine will fail on use.
/// \ingroup ioda_cxx_engines_pub
enum class Capability_Mask {
  /// \brief The feature always works.
  Supported,
  /// \brief The feature causes an exception if used.
  Unsupported,
  /// \brief The feature is silently disabled or unimplemented.
  /// 
  /// \details For example, not all engines support data chunking. If a caller
  ///   specifies that chunking is requested, then we store the
  ///   chunking parameters but do not actually chunk the data.
  ///   Useful when we copy data across backends --- some backends may
  ///   support and use the feature, so we preserve the settings without
  ///   always obeying them.
  Ignored
};

/// \brief Struct defining what an engine can/cannot do.
/// \ingroup ioda_cxx_engines_pub
/// \note These options may vary depending on how ioda-engines
///   and its required libraries are compiled. For example,
///   if SZIP is not available, then the HDF5 backend cannot
///   use SZIP compression.
struct Capabilities {
  Capability_Mask canChunk            = Capability_Mask::Ignored;
  Capability_Mask canCompressWithGZIP = Capability_Mask::Ignored;
  Capability_Mask canCompressWithSZIP = Capability_Mask::Ignored;
  Capability_Mask MPIaware            = Capability_Mask::Unsupported;

  // Other candidate capabilities:
  // canResizeAnyDimension
  // canResize
  // canUseArrayTypes
};
}  // namespace Engines
}  // namespace ioda

/// @}
