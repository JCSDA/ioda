#pragma once
/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

/// \file iodaNamespaceDoc.h
/// \brief This file defines some Doxygen namespace comments. No need to include in your code.

/// \defgroup ioda IODA
/// \brief Everything IODA goes here!

/// \defgroup ioda_language_interfaces Interfaces
/// \brief Per-language interfaces are documented here.
/// \ingroup ioda

/// \defgroup ioda_cxx C++ interface
/// \brief Provides the C++ interface for IODA
/// \ingroup ioda_language_interfaces

/// \defgroup ioda_c C interface
/// \brief Provides the C interface for ioda.
/// \ingroup ioda_language_interfaces

/// \defgroup ioda_fortran Fortran interface
/// \brief The IODA Fortran interface (WIP)
/// \ingroup ioda_language_interfaces

/// \defgroup ioda_python Python interface
/// \brief The IODA Python interface (WIP)
/// \ingroup ioda_language_interfaces

/// \defgroup ioda_cxx_api API
/// \brief C++ API
/// \ingroup ioda_cxx

/// \defgroup ioda_fortran_api API
/// \brief Fortran API
/// \ingroup ioda_fortran

/// \defgroup ioda_engines_grp Engines
/// \brief The powerhouses of IODA
/// \ingroup ioda

/// \defgroup ioda_engines_grp_HH HDF5 / HDFforHumans
/// \brief History and implementation of the HH backend.
/// \ingroup ioda_engines_grp

/// \defgroup ioda_engines_grp_adding Adding a Backend Engine (Advanced)
/// \brief How are backend engines implemented?
/// \ingroup ioda_engines_grp

/// \defgroup ioda_internals Internals
/// \brief Details internal to IODA
/// \ingroup ioda

/// \defgroup ioda_internals_engines Engines
/// \brief Details internal to IODA's engines
/// \ingroup ioda_internals

/// \defgroup ioda_internals_engines_types Types Across Interface Boundaries
/// \brief Details internal to IODA's engines
/// \ingroup ioda_internals_engines


#ifdef __cplusplus

/// \brief The root-level namespace for the ioda library.
namespace ioda {
/// \brief Implementation details. Regular users should not use objects in this class
/// directly, though several user-intended classes inherit from objects here.
namespace detail {}

}  // namespace ioda

#endif  // __cplusplus
