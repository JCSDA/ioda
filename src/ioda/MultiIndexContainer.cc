/*
 * (C) Copyright 2017 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/MultiIndexContainer.h"

namespace ioda {
// -----------------------------------------------------------------------------
  ObsSpaceContainer::ObsSpaceContainer(const eckit::Configuration & config) {
    oops::Log::trace() << "ioda::ObsSpaceContainer Constructor starts " << std::endl;
  }
// -----------------------------------------------------------------------------
  ObsSpaceContainer::~ObsSpaceContainer() {
    oops::Log::trace() << "ioda::ObsSpaceContainer deconstructed " << std::endl;
  }
// -----------------------------------------------------------------------------
  void ObsSpaceContainer::CreateFromFile(const std::string & filename, const std::string & mode,
                                         const util::DateTime & bgn, const util::DateTime & end,
                                         const double & missingvalue,
                                        const eckit::mpi::Comm & commMPI) {
    oops::Log::trace() << "ioda::ObsSpaceContainer opening file: " << filename << std::endl;

    fileio.reset(ioda::IodaIOfactory::Create(filename, mode, bgn, end, missingvalue, commMPI));
    vectors_.reserve(fileio->nvars()*10);

    oops::Log::trace() << "ioda::ObsSpaceContainer opening file ends " << std::endl;
  }
// -----------------------------------------------------------------------------
  void ObsSpaceContainer::LoadData() {
    oops::Log::trace() << "ioda::ObsSpaceContainer loading data starts " << std::endl;
    for (auto iter = (fileio->varlist())->begin(); iter != (fileio->varlist())->end(); ++iter) {
      read_var<double>(std::get<1>(*iter), std::get<0>(*iter));
    }
    oops::Log::trace() << "ioda::ObsSpaceContainer loading data ends " << std::endl;
  }
// -----------------------------------------------------------------------------

bool ObsSpaceContainer::has(const std::string & group, const std::string & name) const {
  auto var = DataContainer.find(boost::make_tuple(group, name));
  return (var != DataContainer.end());
}

// -----------------------------------------------------------------------------

}  // namespace ioda
