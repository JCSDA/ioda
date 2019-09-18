/*
 * (C) Copyright 2017-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/ObsSpace.h"

#include <memory>
#include <string>
#include <vector>
#include <numeric>

#include "eckit/config/Configuration.h"

#include "ioda/ObsData.h"

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
ObsSpace::ObsSpace(const eckit::Configuration & config,
                   const util::DateTime & bgn, const util::DateTime & end)
  : oops::ObsSpaceBase(config, bgn, end),
    obsspace_(new ObsData(config, bgn, end)),
    localobs_(obsspace_->nlocs())
{
  oops::Log::trace() << "Initializing ioda::ObsSpace" << std::endl;
  std::iota(localobs_.begin(), localobs_.end(), 0);
}

ObsSpace::ObsSpace(const ObsSpace & os,
                   const eckit::geometry::Point2 & point,
                   const double & dist,
                   const int & nobs)
  : oops::ObsSpaceBase(os.getConfig(), os.windowStart(), os.windowEnd()),
    obsspace_(os.obsspace_),
    localobs_(),
    refPoint_(point), searchDist_(dist), searchMaxNobs_(nobs)
{
  oops::Log::trace() << "Initializing ioda::ObsSpace for LocalObs" << std::endl;

  std::size_t nlocs_ = obsspace_->nlocs();
  std::vector<float> lats_(nlocs_);
  std::vector<float> lons_(nlocs_);

  eckit::geometry::Point3 refPointxyz_;
  eckit::geometry::Point3 searchPointxyz_;

  eckit::geometry::UnitSphere::convertSphericalToCartesian(refPoint_, refPointxyz_);

  // Get latitudes and longitudes of all observations.
  this -> get_db("MetaData", "longitude", nlocs_, lons_.data());
  this -> get_db("MetaData", "latitude",  nlocs_, lats_.data());

  double localDist;
  double dx;
  for (std::size_t jj=0; jj<nlocs_; ++jj){
    eckit::geometry::Point2 searchPoint_(lons_[jj], lats_[jj]);
    eckit::geometry::UnitSphere::convertSphericalToCartesian(searchPoint_, searchPointxyz_);

    localDist = 0;
    for (std::size_t dd=0; dd<3; ++dd){
      dx = refPointxyz_[dd] - searchPointxyz_[dd];
      localDist += dx * dx;
    }

    if ( std::sqrt(localDist) <= searchDist_ ){
        localobs_.push_back(jj);
    }
  }
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
}

void ObsSpace::get_db(const std::string & group, const std::string & name,
                      const std::size_t vsize, int vdata[]) const {
  obsspace_->get_db(group, name, vsize, vdata);
}

void ObsSpace::get_db(const std::string & group, const std::string & name,
                      const std::size_t vsize, float vdata[]) const {
  obsspace_->get_db(group, name, vsize, vdata);
}

void ObsSpace::get_db(const std::string & group, const std::string & name,
                      const std::size_t vsize, double vdata[]) const {
  obsspace_->get_db(group, name, vsize, vdata);
}

void ObsSpace::get_db(const std::string & group, const std::string & name,
                      const std::size_t vsize, util::DateTime vdata[]) const {
  obsspace_->get_db(group, name, vsize, vdata);
}

void ObsSpace::put_db(const std::string & group, const std::string & name,
                      const std::size_t vsize, const int vdata[]) {
  obsspace_->put_db(group, name, vsize, vdata);
}

void ObsSpace::put_db(const std::string & group, const std::string & name,
                      const std::size_t vsize, const float vdata[]) {
  obsspace_->put_db(group, name, vsize, vdata);
}

void ObsSpace::put_db(const std::string & group, const std::string & name,
                      const std::size_t vsize, const double vdata[]) {
  obsspace_->put_db(group, name, vsize, vdata);
}

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
  return obsspace_->nlocs();
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
