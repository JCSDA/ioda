/*
 * (C) Copyright 2017-2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef OBSSPACE_H_
#define OBSSPACE_H_

#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "eckit/geometry/Point2.h"
#include "eckit/mpi/Comm.h"

#include "ioda/core/ObsData.h"
#include "ioda/distribution/Distribution.h"
#include "ioda/distribution/DistributionFactory.h"
#include "oops/base/ObsSpaceBase.h"
#include "oops/base/Variables.h"
#include "oops/util/DateTime.h"


// Forward declarations
namespace eckit {
  class Configuration;
}

namespace ioda {
  class LocalObsSpaceParameters;
  class ObsVector;

/// Observation Space View
class ObsSpace : public oops::ObsSpaceBase {
 public:
  static const std::string classname() {return "ioda::ObsSpace";}
  typedef std::map<std::size_t, std::vector<std::size_t>> RecIdxMap;
  typedef RecIdxMap::const_iterator RecIdxIter;

  ObsSpace(const eckit::Configuration &, const eckit::mpi::Comm &,
           const util::DateTime &, const util::DateTime &, const eckit::mpi::Comm &);
  ObsSpace(const ObsSpace &, const eckit::geometry::Point2 &,
           const eckit::Configuration &);
  /*!
   * \details Copy constructor for an ObsSpace object.
   */
  ObsSpace(const ObsSpace &) = delete;
  ObsSpace& operator=(const ObsSpace &) = delete;
  ~ObsSpace();

  std::size_t globalNumLocs() const;
  std::size_t nlocs() const;
  std::size_t nlocspatch() const;
  std::size_t nrecs() const;
  std::size_t nvars() const;
  const std::vector<std::size_t> & recnum() const;
  const std::vector<std::size_t> & index() const;

  bool has(const std::string &, const std::string &) const;
  ObsDtype dtype(const std::string &, const std::string &) const;

  const std::vector<std::string> & obs_group_vars() const;
  std::string obs_sort_var() const;
  std::string obs_sort_order() const;

  void get_db(const std::string & group, const std::string & name,
              std::vector<int> & vdata) const;
  void get_db(const std::string & group, const std::string & name,
              std::vector<float> & vdata) const;
  void get_db(const std::string & group, const std::string & name,
              std::vector<double> & vdata) const;
  void get_db(const std::string & group, const std::string & name,
              std::vector<std::string> & vdata) const;
  void get_db(const std::string & group, const std::string & name,
              std::vector<util::DateTime> & vdata) const;

  void put_db(const std::string & group, const std::string & name,
              const std::vector<int> & vdata);
  void put_db(const std::string & group, const std::string & name,
              const std::vector<float> & vdata);
  void put_db(const std::string & group, const std::string & name,
              const std::vector<double> & vdata);
  void put_db(const std::string & group, const std::string & name,
              const std::vector<std::string> & vdata);
  void put_db(const std::string & group, const std::string & name,
              const std::vector<util::DateTime> & vdata);

  bool obsAreSorted() const { return obsspace_->obsAreSorted(); }
  const RecIdxIter recidx_begin() const;
  const RecIdxIter recidx_end() const;
  bool recidx_has(const std::size_t RecNum) const;
  std::size_t recidx_recnum(const RecIdxIter & Irec) const;
  const std::vector<std::size_t> & recidx_vector(const RecIdxIter & Irec) const;
  const std::vector<std::size_t> & recidx_vector(const std::size_t RecNum) const;
  std::vector<std::size_t> recidx_all_recnums() const;
  const std::vector<std::size_t> & localobs() const { return localobs_; }

  /*! \details This method will return the name of the obs type being stored */
  const std::string & obsname() const {return obsspace_->obsname();}
  /*! \details This method will return the start of the DA timing window */
  const util::DateTime & windowStart() const {return obsspace_->windowStart();}
  /*! \details This method will return the end of the DA timing window */
  const util::DateTime & windowEnd() const {return obsspace_->windowEnd();}
  /*! \details This method will return the associated MPI communicator */
  const eckit::mpi::Comm & comm() const {return obsspace_->comm();}

  void printJo(const ObsVector &, const ObsVector &);  // to be removed

  const oops::Variables & obsvariables() const {return obsspace_->obsvariables();}
  const std::vector<double> & obsdist() const {return obsdist_;}

  const Distribution & distribution() const;

 private:
  void print(std::ostream &) const;

  std::shared_ptr<ObsData> obsspace_;
  std::unique_ptr<LocalObsSpaceParameters> localopts_;
  std::vector<std::size_t> localobs_;
  bool isLocal_;
  std::vector<double> obsdist_;
  // for local ObsSpace, all local obs reside on this PE
  // so we will make an Ineffecient distribution that reflects this
  std::shared_ptr<Distribution> localDistribution_;
};

}  // namespace ioda

#endif  // OBSSPACE_H_
