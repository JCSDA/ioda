/*
 * (C) Copyright 2017-2019 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef IODA_OBSSPACE_H_
#define IODA_OBSSPACE_H_

#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "eckit/geometry/Point2.h"
#include "eckit/mpi/Comm.h"

#include "ioda/ObsData.h"
#include "oops/base/Variables.h"
#include "oops/util/DateTime.h"

// Forward declarations
namespace eckit {
  class Configuration;
}

namespace ioda {
  class ObsVector;

/// Observation Space View
class ObsSpace : public util::Printable {
 public:
  static const std::string classname() {return "ioda::ObsSpace";}
  typedef std::map<std::size_t, std::vector<std::size_t>> RecIdxMap;
  typedef RecIdxMap::const_iterator RecIdxIter;

  ObsSpace(const eckit::Configuration &, const eckit::mpi::Comm &,
           const util::DateTime &, const util::DateTime &);
  ObsSpace(const ObsSpace &, const eckit::geometry::Point2 &,
           const double &, const int &);
  /*!
   * \details Copy constructor for an ObsSpace object.
   */
  ObsSpace(const ObsSpace &);
  ~ObsSpace();

  std::size_t gnlocs() const;
  std::size_t nlocs() const;
  std::size_t nrecs() const;
  std::size_t nvars() const;
  const std::vector<std::size_t> & recnum() const;
  const std::vector<std::size_t> & index() const;

  bool has(const std::string &, const std::string &) const;

  void get_db(const std::string & group, const std::string & name,
              const std::size_t vsize, int vdata[]) const;
  void get_db(const std::string & group, const std::string & name,
              const std::size_t vsize, float vdata[]) const;
  void get_db(const std::string & group, const std::string & name,
              const std::size_t vsize, double vdata[]) const;
  void get_db(const std::string & group, const std::string & name,
              const std::size_t vsize, util::DateTime vdata[]) const;

  void put_db(const std::string & group, const std::string & name,
              const std::size_t vsize, const int vdata[]);
  void put_db(const std::string & group, const std::string & name,
              const std::size_t vsize, const float vdata[]);
  void put_db(const std::string & group, const std::string & name,
              const std::size_t vsize, const double vdata[]);
  void put_db(const std::string & group, const std::string & name,
              const std::size_t vsize, const util::DateTime vdata[]);

  const RecIdxIter recidx_begin() const;
  const RecIdxIter recidx_end() const;
  bool recidx_has(const std::size_t RecNum) const;
  std::size_t recidx_recnum(const RecIdxIter & Irec) const;
  const std::vector<std::size_t> & recidx_vector(const RecIdxIter & Irec) const;
  const std::vector<std::size_t> & recidx_vector(const std::size_t RecNum) const;
  std::vector<std::size_t> recidx_all_recnums() const;

  /*! \details This method will return the name of the obs type being stored */
  const std::string & obsname() const {return obsspace_->obsname();}
  /*! \details This method will return the start of the DA timing window */
  const util::DateTime & windowStart() const {return obsspace_->windowStart();}
  /*! \details This method will return the end of the DA timing window */
  const util::DateTime & windowEnd() const {return obsspace_->windowEnd();}
  /*! \details This method will return the associated MPI communicator */
  const eckit::mpi::Comm & comm() const {return obsspace_->comm();}

  void generateDistribution(const eckit::Configuration &);

  void printJo(const ObsVector &, const ObsVector &);  // to be removed

  const oops::Variables & obsvariables() const {return obsspace_->obsvariables();}

 private:
  void print(std::ostream &) const;

  std::shared_ptr<ObsData> obsspace_;
  std::vector<std::size_t> localobs_;
  bool isLocal_;
};

}  // namespace ioda

#endif  // IODA_OBSSPACE_H_
