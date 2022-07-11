/*
 * (C) Copyright 2017-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef IO_OBSIOGENERATEUTILS_H_
#define IO_OBSIOGENERATEUTILS_H_

#include <string>
#include <vector>

/// \file ObsIoGenerateUtils.h
///
/// Utilities used by the ObsIoGenerate* classes

namespace ioda {

class ObsGroup;

/// \brief store generated data into an ObsGroup
/// \param latVals vector of latitude values
/// \param lonVals vector of longitude values
/// \param dts vector of time offsets (s) relative to \p epoch
/// \param epoch (ISO 8601 string) relative to which datetimes are computed
/// \param obsVarNames vector (string) of simulated variable names
/// \param obsErrors vector of obs error estimates
/// \param[out] obsGroup destination for the generated data
void storeGenData(const std::vector<float> & latVals,
                  const std::vector<float> & lonVals,
                  const std::vector<int64_t> & dts,
                  const std::string & epoch,
                  const std::vector<std::string> & obsVarNames,
                  const std::vector<float> & obsErrors,
                  ObsGroup &obsGroup);

}  // namespace ioda

#endif  // IO_OBSIOGENERATEUTILS_H_
