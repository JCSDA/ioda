/*
 * (C) Copyright 2017 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/ObsSpace.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "eckit/config/Configuration.h"
#include "oops/parallel/mpi/mpi.h"
#include "oops/util/abor1_cpp.h"
#include "oops/util/DateTime.h"
#include "oops/util/Logger.h"

#include "distribution/DistributionFactory.h"
#include "fileio/IodaIO.h"
#include "fileio/IodaIOfactory.h"

namespace ioda {

// -----------------------------------------------------------------------------

ObsSpace::ObsSpace(const eckit::Configuration & config,
                   const util::DateTime & bgn, const util::DateTime & end)
  : oops::ObsSpaceBase(config, bgn, end),
    winbgn_(bgn), winend_(end), commMPI_(oops::mpi::comm()),
    database_()
{
  oops::Log::trace() << "ioda::ObsSpace config  = " << config << std::endl;

  obsname_ = config.getString("ObsType");

  // Open the file for input
  std::string filename = config.getString("ObsData.ObsDataIn.obsfile");
  oops::Log::trace() << obsname_ << " file in = " << filename << std::endl;

  InitFromFile(filename, "r", windowStart(), windowEnd());

  // Set the number of locations ,variables and number of observation points
//  nlocs_ = database_.nlocs();
//  nvars_ = database_.nvars();

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
    ss << "_" << std::setw(4) << std::setfill('0') << comm().rank();

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

  oops::Log::trace() << "ioda::ObsSpace contructed name = " << obsname() << std::endl;
}

// -----------------------------------------------------------------------------

ObsSpace::~ObsSpace() {
  if (fileout_.size() != 0) {
    oops::Log::info() << obsname() << ": save database to " << fileout_ << std::endl;
    SaveToFile(fileout_);
  } else {
    oops::Log::info() << obsname() << " :  no output" << std::endl;
  }
}

// -----------------------------------------------------------------------------

template <typename DATATYPE>
void ObsSpace::get_db(const std::string & group, const std::string & name,
                      const size_t & vsize, DATATYPE vdata[]) const {
  std::string gname = (group.size() <= 0)? "GroupUndefined" : group;
  //database_.inquire(gname, name, vsize, vdata);
}

template void ObsSpace::get_db<int>(const std::string & group, const std::string & name,
                                    const size_t & vsize, int vdata[]) const;

template void ObsSpace::get_db<float>(const std::string & group, const std::string & name,
                                      const size_t & vsize, float vdata[]) const;

template void ObsSpace::get_db<double>(const std::string & group, const std::string & name,
                                       const size_t & vsize, double vdata[]) const;

template void ObsSpace::get_db<util::DateTime>(const std::string & group, const std::string & name,
                                               const size_t & vsize, util::DateTime vdata[]) const;

// -----------------------------------------------------------------------------

template <typename DATATYPE>
void ObsSpace::put_db(const std::string & group, const std::string & name,
                      const std::size_t & vsize, const DATATYPE vdata[]) {
  std::string gname = (group.size() <= 0)? "GroupUndefined" : group;
  // database_.insert<DATATYPE>(gname, name, vsize, vdata);
}

template void ObsSpace::put_db<int>(const std::string & group, const std::string & name,
                                    const size_t & vsize, const int vdata[]);

template void ObsSpace::put_db<float>(const std::string & group, const std::string & name,
                                      const size_t & vsize, const float vdata[]);

template void ObsSpace::put_db<double>(const std::string & group, const std::string & name,
                                       const size_t & vsize, const double vdata[]);

// -----------------------------------------------------------------------------

bool ObsSpace::has(const std::string & group, const std::string & name) const {
  return database_.has(group, name);
}
// -----------------------------------------------------------------------------

std::size_t ObsSpace::nlocs() const {
  return nlocs_;
}

// -----------------------------------------------------------------------------

std::size_t ObsSpace::nvars() const {
  return nvars_;
}

// -----------------------------------------------------------------------------

void ObsSpace::generateDistribution(const eckit::Configuration & conf) {
  int fvlen  = conf.getInt("nobs");
  float lat  = conf.getFloat("lat");
  float lon1 = conf.getFloat("lon1");
  float lon2 = conf.getFloat("lon2");

  // Apply the round-robin distribution, which yields the size and indices that
  // are to be selected by this process element out of the file.
  DistributionFactory * distFactory;
  Distribution * dist{distFactory->createDistribution("roundrobin")};
  dist->distribution(comm(), fvlen);
  int nlocs = dist->size();

  // For now, set nvars to one.
  int nvars = 1;

  // Record obs type
  std::string MyObsType = conf.getString("ObsType");
  oops::Log::info() << obsname() << " : " << MyObsType << std::endl;

  // Create variables and generate the values specified by the arguments.
  std::unique_ptr<double[]> latitude {new double[nlocs]};
  for (std::size_t ii = 0; ii < nlocs; ++ii) {
    latitude.get()[ii] = static_cast<double>(lat);
  }
  put_db("", "latitude", nlocs, latitude.get());

  std::unique_ptr<double[]> longitude {new double[nlocs]};
  for (std::size_t ii = 0; ii < nlocs; ++ii) {
    longitude.get()[ii] = static_cast<double>(lon1 + (ii-1)*(lon2-lon1)/(nlocs-1));
  }
  put_db("", "longitude", nlocs, longitude.get());
}

// -----------------------------------------------------------------------------

void ObsSpace::print(std::ostream & os) const {
  os << "ObsSpace::print not implemented";
}

// -----------------------------------------------------------------------------

  void ObsSpace::InitFromFile(const std::string & filename, const std::string & mode,
                              const util::DateTime &, const util::DateTime &) {
    oops::Log::trace() << "ioda::ObsSpace opening file: " << filename << std::endl;

    std::unique_ptr<IodaIO> fileio {ioda::IodaIOfactory::Create(filename, mode)};
    nlocs_ = fileio->nlocs();
    nvars_ = fileio->nvars();

    // Load all valid variables
    std::unique_ptr<boost::any[]> vect;
    std::string group, variable;

    for (IodaIO::GroupIter igrp = fileio->group_begin();
                           igrp != fileio->group_end(); ++igrp) {
      for (IodaIO::VarIter ivar = fileio->var_begin(igrp);
                           ivar != fileio->var_end(igrp); ++ivar) {
        // Revisit here, improve the readability
        group = fileio->group_name(igrp);
        variable = fileio->var_name(ivar);
        vect.reset(new boost::any[nlocs()]);
//        fileio->ReadVar(group, variable, vect.get());
        // All records read from file are read-only
//        database_.container()->insert({group, variable, "r", nlocs(), vect});
      }
    }
    oops::Log::trace() << "ioda::ObsSpaceContainer opening file ends " << std::endl;
  }

// -----------------------------------------------------------------------------

  void ObsSpace::SaveToFile(const std::string & file_name) {
    // Open the file for output
    std::unique_ptr<IodaIO> fileio
      {ioda::IodaIOfactory::Create(file_name, "W", nlocs(), 0, nvars())};

    // List all records and write out the every record
//    for (ObsSpaceContainer::VarIter iter = database_.var_iter_begin();
//         iter != database_.var_iter_end(); ++iter) {
// std::cout << "DEBUG: SaveToFile: gname, vname, data: " << database_.var_iter_gname(iter)
//           << ", " << database_.var_iter_vname(iter)
//           << ", " << database_.var_iter_data(iter) << std::endl;
//      fileio->WriteVar(database_.var_iter_gname(iter),
//                       database_.var_iter_vname(iter), database_.var_iter_data(iter));
//    }
  }

// -----------------------------------------------------------------------------

void ObsSpace::printJo(const ObsVector & dy, const ObsVector & grad) {
  oops::Log::info() << "ObsSpace::printJo not implemented" << std::endl;
}

// -----------------------------------------------------------------------------

}  // namespace ioda
