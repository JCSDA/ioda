/*
 * (C) Copyright 2017-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/ObsSpace.h"

#include <algorithm>
#include <memory>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

#include "eckit/config/Configuration.h"
#include "eckit/geometry/Point3.h"
#include "eckit/geometry/UnitSphere.h"

#include "oops/util/abor1_cpp.h"
#include "oops/util/DateTime.h"
#include "oops/util/Duration.h"
#include "oops/util/Logger.h"

namespace ioda {

// -----------------------------------------------------------------------------
/*!
 * \details Config based constructor for an ObsSpace object. This constructor will read
 *          in from the obs file and transfer the variables into the obs container. Obs
 *          falling outside the DA timing window, specified by bgn and end, will be
 *          discarded before storing them in the obs container.
 *
 * \param[in] config ECKIT configuration segment holding obs types specs
 * \param[in] bgn    DateTime object holding the start of the DA timing window
 * \param[in] end    DateTime object holding the end of the DA timing window
 */
ObsSpace::ObsSpace(const eckit::Configuration & config, const eckit::mpi::Comm & comm,
                   const util::DateTime & bgn, const util::DateTime & end)
  : obsspace_(new ObsData(config, comm, bgn, end)),
    localobs_(obsspace_->nlocs()), isLocal_(false)
{
  oops::Log::trace() << "ioda::ObsSpaces starting" << std::endl;
  std::iota(localobs_.begin(), localobs_.end(), 0);
  oops::Log::trace() << "ioda::ObsSpace done" << std::endl;
}

// -----------------------------------------------------------------------------
/*!
 * \details Second constructor for an ObsSpace object.
 *          This constructor will search for observations within a (dist) from a
 *          specified (Point2).  The number of locations will be limited to (nobs)
 */
ObsSpace::ObsSpace(const ObsSpace & os,
                   const eckit::geometry::Point2 & refPoint,
                   const double & maxDist,
                   const int & maxNobs)
  : obsspace_(os.obsspace_),
    localobs_(), isLocal_(true)
{
  oops::Log::trace() << "ioda::ObsSpace for LocalObs starting" << std::endl;

  const std::string searchMethod = "brute_force";  //  hard-wired for now!

  std::vector<double> obsdist;

  if ( searchMethod == "brute_force" ) {
    oops::Log::trace() << "ioda::ObsSpace searching via brute force" << std::endl;

    std::size_t nlocs = obsspace_->nlocs();
    std::vector<float> lats(nlocs);
    std::vector<float> lons(nlocs);

    // Get latitudes and longitudes of all observations.
    obsspace_ -> get_db("MetaData", "longitude", nlocs, lons.data());
    obsspace_ -> get_db("MetaData", "latitude",  nlocs, lats.data());

    const double radiusEarth = 6.371e6;
    for (unsigned int jj = 0; jj < nlocs; ++jj) {
      eckit::geometry::Point2 searchPoint(lons[jj], lats[jj]);
      double localDist = eckit::geometry::Sphere::distance(radiusEarth, refPoint, searchPoint);
      if ( localDist < maxDist ) {
        localobs_.push_back(jj);
        obsdist.push_back(localDist);
      }
    }
  } else {
    oops::Log::trace() << "ioda::ObsSpace searching via KDTree" << std::endl;

    std::string ErrMsg =
      std::string("ioda::ObsSpace search via KDTree not implemented,") +
      std::string("use 'brute_force' for 'searchMethod:' YAML configuration keyword.");
    ABORT(ErrMsg);
  }

  if ( (maxNobs > 0) && (localobs_.size() > maxNobs) ) {
    for (unsigned int jj = 0; jj < localobs_.size(); ++jj) {
        oops::Log::debug() << "Before sort [i, d]: " << localobs_[jj]
            << " , " << obsdist[jj] << std::endl;
    }

    // Construct a temporary paired vector to do the sorting
    std::vector<std::pair<std::size_t, double>> localObsIndDistPair;
    for (unsigned int jj = 0; jj < obsdist.size(); ++jj) {
      localObsIndDistPair.push_back(std::make_pair(localobs_[jj], obsdist[jj]));
    }

    // Use a lambda function to implement an ascending sort.
    sort(localObsIndDistPair.begin(), localObsIndDistPair.end(),
         [](const std::pair<std::size_t, double> & p1,
            const std::pair<std::size_t, double> & p2){
              return(p1.second < p2.second);
            });

    // Unpair the sorted pair vector
    for (unsigned int jj = 0; jj < obsdist.size(); ++jj) {
      localobs_[jj] = localObsIndDistPair[jj].first;
      obsdist[jj] = localObsIndDistPair[jj].second;
    }

    // Truncate to maxNobs length
    localobs_.resize(maxNobs);
    obsdist.resize(maxNobs);

    for (unsigned int jj = 0; jj < localobs_.size(); ++jj) {
        oops::Log::debug() << " After sort [i, d]: " << localobs_[jj] << " , "
            << obsdist[jj] << std::endl;
    }
  }

  oops::Log::trace() << "ioda::ObsSpace for LocalObs done" << std::endl;
}

// -----------------------------------------------------------------------------
/*!
 * \details Destructor for an ObsSpace object. This destructor will clean up the ObsSpace
 *          object and optionally write out the contents of the obs container into
 *          the output file. The save-to-file operation is invoked when an output obs
 *          file is specified in the ECKIT configuration segment associated with the
 *          ObsSpace object.
 */
ObsSpace::~ObsSpace() {
  oops::Log::trace() << "ioda::~ObsSpace done" << std::endl;
}

// -----------------------------------------------------------------------------

void ObsSpace::get_db(const std::string & group, const std::string & name,
                      const std::size_t vsize, int vdata[]) const {
  if ( isLocal_ ) {
    std::vector<int> vdataTmp(obsspace_ -> nlocs());
    obsspace_->get_db(group, name, vdataTmp.size(), vdataTmp.data());
    for (unsigned int ii = 0; ii < localobs_.size(); ++ii) {
      vdata[ii] = vdataTmp[localobs_[ii]];
    }
  } else {
    obsspace_->get_db(group, name, vsize, vdata);
  }
}

// -----------------------------------------------------------------------------

void ObsSpace::get_db(const std::string & group, const std::string & name,
                      const std::size_t vsize, float vdata[]) const {
  if ( isLocal_ ) {
    std::vector<float> vdataTmp(obsspace_->nlocs());
    obsspace_->get_db(group, name, vdataTmp.size(), vdataTmp.data());
    for (unsigned int ii = 0; ii < localobs_.size(); ++ii) {
      vdata[ii] = vdataTmp[localobs_[ii]];
    }
  } else {
    obsspace_->get_db(group, name, vsize, vdata);
  }
}

// -----------------------------------------------------------------------------

void ObsSpace::get_db(const std::string & group, const std::string & name,
                      const std::size_t vsize, double vdata[]) const {
  if ( isLocal_ ) {
    std::vector<double> vdataTmp(obsspace_->nlocs());
    obsspace_->get_db(group, name, vdataTmp.size(), vdataTmp.data());
    for (unsigned int ii = 0; ii < localobs_.size(); ++ii) {
      vdata[ii] = vdataTmp[localobs_[ii]];
    }
  } else {
    obsspace_->get_db(group, name, vsize, vdata);
  }
}

// -----------------------------------------------------------------------------

void ObsSpace::get_db(const std::string & group, const std::string & name,
                      const std::size_t vsize, util::DateTime vdata[]) const {
  if ( isLocal_ ) {
    std::vector<util::DateTime> vdataTmp(obsspace_->nlocs());
    obsspace_->get_db(group, name, vdataTmp.size(), vdataTmp.data());
    for (unsigned int ii = 0; ii < localobs_.size(); ++ii) {
      vdata[ii] = vdataTmp[localobs_[ii]];
    }
  } else {
    obsspace_->get_db(group, name, vsize, vdata);
  }
}

// -----------------------------------------------------------------------------

void ObsSpace::put_db(const std::string & group, const std::string & name,
                      const std::size_t vsize, const int vdata[]) {
  obsspace_->put_db(group, name, vsize, vdata);
}

// -----------------------------------------------------------------------------

void ObsSpace::put_db(const std::string & group, const std::string & name,
                      const std::size_t vsize, const float vdata[]) {
  obsspace_->put_db(group, name, vsize, vdata);
}

// -----------------------------------------------------------------------------

void ObsSpace::put_db(const std::string & group, const std::string & name,
                      const std::size_t vsize, const double vdata[]) {
  obsspace_->put_db(group, name, vsize, vdata);
}

// -----------------------------------------------------------------------------

void ObsSpace::put_db(const std::string & group, const std::string & name,
                      const std::size_t vsize, const util::DateTime vdata[]) {
  obsspace_->put_db(group, name, vsize, vdata);
}

// -----------------------------------------------------------------------------
/*!
 * \details This method checks for the existence of the group, name combination
 *          in the obs container. If the combination exists, "true" is returned,
 *          otherwise "false" is returned.
 */

bool ObsSpace::has(const std::string & group, const std::string & name) const {
  return obsspace_->has(group, name);
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the number of unique locations in the input
 *          obs file. Note that nlocs from the obs container may be smaller
 *          than nlocs from the input obs file due to the removal of obs outside
 *          the DA timing window and/or due to distribution of obs across
 *          multiple process elements.
 */
std::size_t ObsSpace::gnlocs() const {
  return obsspace_->gnlocs();
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the number of unique locations in the obs
 *          container. Note that nlocs from the obs container may be smaller
 *          than nlocs from the input obs file due to the removal of obs outside
 *          the DA timing window and/or due to distribution of obs across
 *          multiple process elements.
 */
std::size_t ObsSpace::nlocs() const {
  if ( isLocal_ ) {
    return localobs_.size();
  } else {
    return obsspace_->nlocs();
  }
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the number of unique records in the obs
 *          container. A record is an atomic unit of locations that belong
 *          together such as a single radiosonde sounding.
 */
std::size_t ObsSpace::nrecs() const {
  return obsspace_->nrecs();
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the number of unique variables in the obs
 *          container. "Variables" refers to the quantities that can be
 *          assimilated as opposed to meta data.
 */
std::size_t ObsSpace::nvars() const {
  return obsspace_->nvars();
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns a reference to the record number vector
 *          data member. This is for read only access.
 */
const std::vector<std::size_t> & ObsSpace::recnum() const {
  return obsspace_->recnum();
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns a reference to the index vector
 *          data member. This is for read only access.
 */
const std::vector<std::size_t> & ObsSpace::index() const {
  return obsspace_->index();
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the begin iterator associated with the
 *          recidx_ data member.
 */
const ObsSpace::RecIdxIter ObsSpace::recidx_begin() const {
  return obsspace_->recidx_begin();
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the end iterator associated with the
 *          recidx_ data member.
 */
const ObsSpace::RecIdxIter ObsSpace::recidx_end() const {
  return obsspace_->recidx_end();
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns a boolean value indicating whether the
 *          given record number exists in the recidx_ data member.
 */
bool ObsSpace::recidx_has(const std::size_t RecNum) const {
  return obsspace_->recidx_has(RecNum);
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the current record number, pointed to by the
 *          given iterator, from the recidx_ data member.
 */
std::size_t ObsSpace::recidx_recnum(const RecIdxIter & Irec) const {
  return obsspace_->recidx_recnum(Irec);
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the current vector, pointed to by the
 *          given iterator, from the recidx_ data member.
 */
const std::vector<std::size_t> & ObsSpace::recidx_vector(const RecIdxIter & Irec) const {
  return obsspace_->recidx_vector(Irec);
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the current vector, pointed to by the
 *          given iterator, from the recidx_ data member.
 */
const std::vector<std::size_t> & ObsSpace::recidx_vector(const std::size_t RecNum) const {
  return obsspace_->recidx_vector(RecNum);
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the all of the record numbers from the
 *          recidx_ data member (ie, all the key values) in a vector.
 */
std::vector<std::size_t> ObsSpace::recidx_all_recnums() const {
  return obsspace_->recidx_all_recnums();
}

// -----------------------------------------------------------------------------
/*!
 * \details This method will generate a set of latitudes and longitudes of which
 *          can be used for testing without reading in an obs file. Two latitude
 *          values, two longitude values, the number of locations (nobs keyword)
 *          and an optional random seed are specified in the configuration given
 *          by the conf parameter. Random locations between the two latitudes and
 *          two longitudes are generated and stored in the obs container as meta data.
 *          Random time stamps that fall inside the given timing window (which is
 *          specified in the configuration file) are alos generated and stored
 *          in the obs container as meta data.
 *
 * \param[in] conf ECKIT configuration segment built from an input configuration file.
 */
void ObsSpace::generateDistribution(const eckit::Configuration & conf) {
  obsspace_->generateDistribution(conf);
}

// -----------------------------------------------------------------------------
/*!
 * \details This method provides a way to print an ObsSpace object in an output
 *          stream. It simply prints a dummy message for now.
 */
void ObsSpace::print(std::ostream & os) const {
  os << *obsspace_;
}


// -----------------------------------------------------------------------------
/*!
 * \details This method provides a means for printing Jo in
 *          an output stream. For now a dummy message is printed.
 */
void ObsSpace::printJo(const ObsVector & dy, const ObsVector & grad) {
  obsspace_->printJo(dy, grad);
}

// -----------------------------------------------------------------------------

}  // namespace ioda
