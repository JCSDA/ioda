/*
 * (C) Copyright 2017-2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/ObsSpace.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "eckit/config/Configuration.h"
#include "eckit/exception/Exceptions.h"
#include "eckit/config/LocalConfiguration.h"

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
                   const util::DateTime & bgn, const util::DateTime & end,
                   const eckit::mpi::Comm & time)
  : oops::ObsSpaceBase(config, comm, bgn, end),
    obsspace_(new ObsData(config, comm, bgn, end, time))
{
  oops::Log::trace() << "ioda::ObsSpace done" << std::endl;
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
  obsspace_->get_db(group, name, vdata);
}

// -----------------------------------------------------------------------------

void ObsSpace::get_db(const std::string & group, const std::string & name,
                      std::vector<float> & vdata) const {
  obsspace_->get_db(group, name, vdata);
}

// -----------------------------------------------------------------------------

void ObsSpace::get_db(const std::string & group, const std::string & name,
                      std::vector<double> & vdata) const {
  obsspace_->get_db(group, name, vdata);
}

// -----------------------------------------------------------------------------

void ObsSpace::get_db(const std::string & group, const std::string & name,
                      std::vector<std::string> & vdata) const {
  obsspace_->get_db(group, name, vdata);
}

// -----------------------------------------------------------------------------

void ObsSpace::get_db(const std::string & group, const std::string & name,
                      std::vector<util::DateTime> & vdata) const {
  obsspace_->get_db(group, name, vdata);
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
 *          parameter: obsdatain.obsgrouping.group variables
 */
const std::vector<std::string> & ObsSpace::obs_group_vars() const {
  return obsspace_->obs_group_vars();
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
 * \details This method returns the number of global unique locations.
 *          Note that nlocs from the obs container may be smaller than
 *          number of global unique locations due to distribution of obs across
 *          multiple process elements.
 */
std::size_t ObsSpace::globalNumLocs() const {
  return obsspace_->globalNumLocs();
}

// -----------------------------------------------------------------------------
/*!
 * \details This method returns the number of locations in the obs
 *          container. This method is used to reserve enough memory to store 
 *          the obs local to this PE. 
 *          Note that nlocs from the obs container may be smaller
 *          than global unique nlocs due to distribution of obs across
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
 *
 * The returned vector has length nlocs() and contains the original indices of locations from the
 * input ioda file corresponding to locations stored in this ObsSpace object -- i.e. those that
 * were selected by the timing window filter and the MPI distribution.
 *
 * Example 1: Suppose the RoundRobin distribution is used and and there are two MPI tasks (ranks 0
 * and 1). The even-numbered locations from the file will go to rank 0, and the odd-numbered
 * locations will go to rank 1. This means that `ObsSpace::index()` will return the vector `0, 2,
 * 4, 6, ...` on rank 0 and `1, 3, 5, 7, ...` on rank 1.
 *
 * Example 2: Suppose MPI is not used and the file contains 10 locations in total, but locations 2,
 * 3 and 7 are outside the DA timing window. In this case, `ObsSpace::index()` will return `0, 1,
 * 4, 5, 6, 8, 9`.
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
}  // namespace ioda
