/*
 * (C) Copyright 2017-2020 UCAR
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

#include "ioda/core/LocalObsSpaceParameters.h"

#include "oops/util/abor1_cpp.h"
#include "oops/util/DateTime.h"
#include "oops/util/Duration.h"
#include "oops/util/Logger.h"

#include "atlas/util/Earth.h"


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
                   const util::DateTime & bgn, const util::DateTime & end,
                   const eckit::mpi::Comm & time)
  : oops::ObsSpaceBase(config, comm, bgn, end),
    localopts_(), obsspace_(new ObsData(config, comm, bgn, end, time)),
    localobs_(obsspace_->nlocs()), isLocal_(false),
    obsdist_()
{
  oops::Log::trace() << "ioda::ObsSpaces starting" << std::endl;
  std::iota(localobs_.begin(), localobs_.end(), 0);
  oops::Log::trace() << "ioda::ObsSpace done" << std::endl;
}

// -----------------------------------------------------------------------------
/*!
 * \details Second constructor for an ObsSpace object.
 *          This constructor will search for observations within a distance specified
 *          in the \p conf from a specified \p refPoint.
 */
ObsSpace::ObsSpace(const ObsSpace & os,
                   const eckit::geometry::Point2 & refPoint,
                   const eckit::Configuration & conf)
  : oops::ObsSpaceBase(eckit::LocalConfiguration(), os.comm(), os.windowStart(), os.windowEnd()),
    localopts_(new LocalObsSpaceParameters()), obsspace_(os.obsspace_),
    localobs_(), isLocal_(true),
    obsdist_()
{
  oops::Log::trace() << "ioda::ObsSpace for LocalObs starting" << std::endl;
  localopts_->deserialize(conf);

  if ( localopts_->searchMethod == SearchMethod::BRUTEFORCE ) {
    oops::Log::trace() << "ioda::ObsSpace searching via brute force" << std::endl;

    std::size_t nlocs = obsspace_->nlocs();
    std::vector<float> lats(nlocs);
    std::vector<float> lons(nlocs);

    // Get latitudes and longitudes of all observations.
    obsspace_ -> get_db("MetaData", "longitude", lons);
    obsspace_ -> get_db("MetaData", "latitude", lats);

    for (unsigned int jj = 0; jj < nlocs; ++jj) {
      eckit::geometry::Point2 searchPoint(lons[jj], lats[jj]);
      double localDist = localopts_->distance(refPoint, searchPoint);
      if ( localDist < localopts_->lengthscale ) {
        localobs_.push_back(jj);
        obsdist_.push_back(localDist);
      }
    }
    const boost::optional<int> & maxnobs = localopts_->maxnobs;
    if ( (maxnobs != boost::none) && (localobs_.size() > *maxnobs ) ) {
      for (unsigned int jj = 0; jj < localobs_.size(); ++jj) {
          oops::Log::debug() << "Before sort [i, d]: " << localobs_[jj]
              << " , " << obsdist_[jj] << std::endl;
      }
      // Construct a temporary paired vector to do the sorting
      std::vector<std::pair<std::size_t, double>> localObsIndDistPair;
      for (unsigned int jj = 0; jj < obsdist_.size(); ++jj) {
        localObsIndDistPair.push_back(std::make_pair(localobs_[jj], obsdist_[jj]));
      }

      // Use a lambda function to implement an ascending sort.
      sort(localObsIndDistPair.begin(), localObsIndDistPair.end(),
           [](const std::pair<std::size_t, double> & p1,
              const std::pair<std::size_t, double> & p2){
                return(p1.second < p2.second);
              });

      // Unpair the sorted pair vector
      for (unsigned int jj = 0; jj < obsdist_.size(); ++jj) {
        localobs_[jj] = localObsIndDistPair[jj].first;
        obsdist_[jj] = localObsIndDistPair[jj].second;
      }

      // Truncate to maxNobs length
      localobs_.resize(*maxnobs);
      obsdist_.resize(*maxnobs);
    }
  } else {
    oops::Log::trace() << "ioda::ObsSpace searching via KDTree" << std::endl;

    if ( localopts_->distanceType == DistanceType::CARTESIAN)
      ABORT("ObsSpace:: search method must be 'brute_force' when using 'cartesian' distance");

    ObsData::KDTree & kdtree = obsspace_->getKDTree();

    // Using the radius of the earth
    eckit::geometry::Point3 refPoint3D;
    atlas::util::Earth::convertSphericalToCartesian(refPoint, refPoint3D);
    double alpha =  (localopts_->lengthscale / localopts_->radius_earth)/ 2.0;  // angle in radians
    double chordLength = 2.0*localopts_->radius_earth * sin(alpha);  // search radius in 3D space

    auto closePoints = kdtree.findInSphere(refPoint3D, chordLength);

    // put closePoints back into localobs_ and obsdist_
    for (unsigned int jj = 0; jj < closePoints.size(); ++jj) {
       localobs_.push_back(closePoints[jj].payload());  // observation
       obsdist_.push_back(closePoints[jj].distance());  // distance
     }

    // The obs are sorted in the kdtree call
    const boost::optional<int> & maxnobs = localopts_->maxnobs;
    if ( (maxnobs != boost::none) && (localobs_.size() > *maxnobs ) ) {
      // Truncate to maxNobs length
      localobs_.resize(*maxnobs);
      obsdist_.resize(*maxnobs);
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
                      std::vector<int> & vdata) const {
  if ( isLocal_ ) {
    std::vector<int> vdataTmp(obsspace_ -> nlocs());
    obsspace_->get_db(group, name, vdataTmp);
    for (unsigned int ii = 0; ii < localobs_.size(); ++ii) {
      vdata[ii] = vdataTmp[localobs_[ii]];
    }
  } else {
    obsspace_->get_db(group, name, vdata);
  }
}

// -----------------------------------------------------------------------------

void ObsSpace::get_db(const std::string & group, const std::string & name,
                      std::vector<float> & vdata) const {
  if ( isLocal_ ) {
    std::vector<float> vdataTmp(obsspace_->nlocs());
    obsspace_->get_db(group, name, vdataTmp);
    for (unsigned int ii = 0; ii < localobs_.size(); ++ii) {
      vdata[ii] = vdataTmp[localobs_[ii]];
    }
  } else {
    obsspace_->get_db(group, name, vdata);
  }
}

// -----------------------------------------------------------------------------

void ObsSpace::get_db(const std::string & group, const std::string & name,
                      std::vector<double> & vdata) const {
  if ( isLocal_ ) {
    std::vector<double> vdataTmp(obsspace_->nlocs());
    obsspace_->get_db(group, name, vdataTmp);
    for (unsigned int ii = 0; ii < localobs_.size(); ++ii) {
      vdata[ii] = vdataTmp[localobs_[ii]];
    }
  } else {
    obsspace_->get_db(group, name, vdata);
  }
}

// -----------------------------------------------------------------------------

void ObsSpace::get_db(const std::string & group, const std::string & name,
                      std::vector<std::string> & vdata) const {
  if ( isLocal_ ) {
    std::vector<std::string> vdataTmp(obsspace_->nlocs());
    obsspace_->get_db(group, name, vdataTmp);
    for (unsigned int ii = 0; ii < localobs_.size(); ++ii) {
      vdata[ii] = vdataTmp[localobs_[ii]];
    }
  } else {
    obsspace_->get_db(group, name, vdata);
  }
}

// -----------------------------------------------------------------------------

void ObsSpace::get_db(const std::string & group, const std::string & name,
                      std::vector<util::DateTime> & vdata) const {
  if ( isLocal_ ) {
    std::vector<util::DateTime> vdataTmp(obsspace_->nlocs());
    obsspace_->get_db(group, name, vdataTmp);
    for (unsigned int ii = 0; ii < localobs_.size(); ++ii) {
      vdata[ii] = vdataTmp[localobs_[ii]];
    }
  } else {
    obsspace_->get_db(group, name, vdata);
  }
}

// -----------------------------------------------------------------------------

void ObsSpace::put_db(const std::string & group, const std::string & name,
                      const std::vector<int> & vdata) {
  obsspace_->put_db(group, name, vdata);
}

// -----------------------------------------------------------------------------

void ObsSpace::put_db(const std::string & group, const std::string & name,
                      const std::vector<float> & vdata) {
  obsspace_->put_db(group, name, vdata);
}

// -----------------------------------------------------------------------------

void ObsSpace::put_db(const std::string & group, const std::string & name,
                      const std::vector<double> & vdata) {
  obsspace_->put_db(group, name, vdata);
}

// -----------------------------------------------------------------------------

void ObsSpace::put_db(const std::string & group, const std::string & name,
                      const std::vector<std::string> & vdata) {
  obsspace_->put_db(group, name, vdata);
}

// -----------------------------------------------------------------------------

void ObsSpace::put_db(const std::string & group, const std::string & name,
                      const std::vector<util::DateTime> & vdata) {
  obsspace_->put_db(group, name, vdata);
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
 * \details This method returns the data type of the variable stored in the obs
 *          container.
 */

ObsDtype ObsSpace::dtype(const std::string & group, const std::string & name) const {
  return obsspace_->dtype(group, name);
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the setting of the YAML configuration
 *          parameter: obsdatain.obsgrouping.group variable
 */
std::string ObsSpace::obs_group_var() const {
  return obsspace_->obs_group_var();
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the setting of the YAML configuration
 *          parameter: obsdatain.obsgrouping.sort variable
 */
std::string ObsSpace::obs_sort_var() const {
  return obsspace_->obs_sort_var();
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the setting of the YAML configuration
 *          parameter: obsdatain.obsgrouping.sort order
 */
std::string ObsSpace::obs_sort_order() const {
  return obsspace_->obs_sort_order();
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
