/*
 * (C) Copyright 2017 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/ObsSpace.h"

#include <map>
#include <string>
#include <vector>

#include "eckit/config/Configuration.h"

#include "oops/parallel/mpi/mpi.h"
#include "oops/util/abor1_cpp.h"
#include "oops/util/DateTime.h"
#include "oops/util/Logger.h"

namespace ioda {
// -----------------------------------------------------------------------------
const double ObsSpace::missingvalue_ = -9.9999e+299;
// -----------------------------------------------------------------------------

ObsSpace::ObsSpace(const eckit::Configuration & config,
                   const util::DateTime & bgn, const util::DateTime & end)
  : oops::ObsSpaceBase(config, bgn, end), winbgn_(bgn), winend_(end),
    commMPI_(oops::mpi::comm()), database_(config)
{
  oops::Log::trace() << "ioda::ObsSpace config  = " << config << std::endl;

  const eckit::Configuration * configc = &config;
  obsname_ = config.getString("ObsType");

  // Open the file for input
  std::string filename = config.getString("ObsData.ObsDataIn.obsfile");
  oops::Log::trace() << obsname_ << " file in = " << filename << std::endl;

  database_.CreateFromFile(filename, "r", bgn, end, missingValue(), commMPI_);

  // Set the number of locations ,variables and number of observation points
  nlocs_ = database_.nlocs();
  nvars_ = database_.nvars();
  nobs_ = nvars_ * nlocs_;

  // Load in all VALID variables
  database_.LoadData();

  // Check to see if an output file has been requested.
  if (config.has("ObsData.ObsDataOut.obsfile")) {
    std::string filename = config.getString("ObsData.ObsDataOut.obsfile");

    // Find the left-most dot in the file name, and use that to pick off the file name
    // and file extension.
    std::size_t found = filename.find_last_of(".");
    if (found == std::string::npos)
      found = filename.length();

    // Get the process rank number and format it
    std::ostringstream ss;
    ss << "_" << std::setw(4) << std::setfill('0') << commMPI_.rank();

    // Construct the output file name
    fileout_ = filename.insert(found, ss.str());

    // Check to see if user is trying to overwrite an existing file. For now always allow
    // the overwrite, but issue a warning if we are about to clobber an existing file.
    std::ifstream infile(fileout_);
    if (infile.good())
      oops::Log::warning() << "ioda::ObsSpace WARNING: Overwriting output file "
                           << fileout_ << std::endl;
  } else {
    oops::Log::debug() << "ioda::ObsSpace output file is not required " << std::endl;
  }

  const util::DateTime * p1 = &winbgn_;
  const util::DateTime * p2 = &winend_;
  ioda_obsdb_setup_f90(keyOspace_, &configc, &p1, &p2, missingvalue_);

  oops::Log::trace() << "ioda::ObsSpace contructed name = " << obsname_ << std::endl;
}

// -----------------------------------------------------------------------------

ObsSpace::~ObsSpace() {
  ioda_obsdb_delete_f90(keyOspace_);
}
// -----------------------------------------------------------------------------
void ObsSpace::get_refdate(util::DateTime & refdate) const {
  ioda_obsdb_getrefdate_f90(keyOspace_, refdate);
}
// -----------------------------------------------------------------------------
template <typename Type>
void ObsSpace::get_db(const std::string & group, const std::string & name,
                      const size_t & vsize, Type vdata[]) const {
  std::string gname(group);
  if (group.size() <= 0)
    gname = "GroupUndefined";
  database_.inquire<Type>(gname, name, vsize, vdata);
}

template void ObsSpace::get_db<int>(const std::string & group, const std::string & name,
                                    const size_t & vsize, int vdata[]) const;

template void ObsSpace::get_db<double>(const std::string & group, const std::string & name,
                                       const size_t & vsize, double vdata[]) const;

// -----------------------------------------------------------------------------
template <typename Type>
void ObsSpace::put_db(const std::string & group, const std::string & name,
                      const std::size_t & vsize, const Type vdata[]) {
  std::string gname(group);
  if (group.size() <= 0)
    gname = "GroupUndefined";
  database_.insert<Type>(gname, name, vsize, vdata);
}

template void ObsSpace::put_db<int>(const std::string & group, const std::string & name,
                                    const size_t & vsize, const int vdata[]);

template void ObsSpace::put_db<double>(const std::string & group, const std::string & name,
                                       const size_t & vsize, const double vdata[]);

// -----------------------------------------------------------------------------
bool ObsSpace::has(const std::string & group, const std::string & name) const {
  return (database_.has(group, name));
}
// -----------------------------------------------------------------------------

std::size_t ObsSpace::nobs() const {
  return nobs_;
}

// -----------------------------------------------------------------------------

std::size_t ObsSpace::nlocs() const {
  return nlocs_;
}

// -----------------------------------------------------------------------------

void ObsSpace::generateDistribution(const eckit::Configuration & conf) {
  const eckit::Configuration * configc = &conf;

  const util::DateTime * p1 = &winbgn_;
  const util::DateTime * p2 = &winend_;
  ioda_obsdb_generate_f90(keyOspace_, &configc, &p1, &p2, missingvalue_);
}

// -----------------------------------------------------------------------------

void ObsSpace::print(std::ostream & os) const {
  os << "ObsSpace::print not implemented";
}

// -----------------------------------------------------------------------------

void ObsSpace::printJo(const ObsVector & dy, const ObsVector & grad) {
  oops::Log::info() << "ObsSpace::printJo not implemented" << std::endl;
}

// -----------------------------------------------------------------------------

}  // namespace ioda
