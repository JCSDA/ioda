/*
 * (C) Copyright 2017-2018 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef IODA_OBSSPACEFACTORY_H_
#define IODA_OBSSPACEFACTORY_H_

#include <map>
#include <string>

#include <boost/noncopyable.hpp>

#include "eckit/config/Configuration.h"
#include "oops/util/abor1_cpp.h"
#include "oops/util/Printable.h"
#include "oops/util/DateTime.h"
#include "oops/interface/ObsSpaceBase.h"

#include "ioda/Locations.h"
#include "ioda/ObsVector.h"

namespace ioda {
  class Locations;
  class ObsVector;

// -----------------------------------------------------------------------------
class ObsSpaceBase : public oops::ObsSpaceBase {
 public:
  // oops::ObsSpaceBase defines:
  //   config()
  //   windowStart()
  //   windowEnd()
  //
  // oops::ObsSpaceBase declares:
  //   generateDistribution()
  //   print()
  //
  // Need to add declarations for:
  //   constructor
  //   destructor
  //   locations()
  //   printJo()

  ObsSpaceBase(const eckit::Configuration & config, const util::DateTime & bgn,
               const util::DateTime & end) : oops::ObsSpaceBase(config, bgn, end) { };
  virtual ~ObsSpaceBase() { };

  virtual Locations* locations(const util::DateTime &, const util::DateTime &) const = 0;
  virtual void printJo(const ObsVector &, const ObsVector &) = 0;

  //virtual void getobs(const std::string &, ObsVector &) const = 0;
  //virtual void putobs(const std::string &, const ObsVector &) const = 0;
  virtual const std::string & obsname() const = 0;
  virtual int nobs() const = 0;

  virtual void getdb(const std::string &, int &) const = 0;
  virtual void putdb(const std::string &, const int &) const = 0;

  virtual void get_mdata(const std::string &, double [], const int) const = 0;

};

// Obs Space Factory
class ObsSpaceFactory {
 public:
  static ObsSpaceBase * create(const eckit::Configuration &,
                               const util::DateTime &, const util::DateTime &);
  virtual ~ObsSpaceFactory() { getMakers().clear(); }
 protected:
  explicit ObsSpaceFactory(const std::string &);
 private:
  virtual ObsSpaceBase * make(const eckit::Configuration &,
                              const util::DateTime &, const util::DateTime &) = 0;
  static std::map < std::string, ObsSpaceFactory * > & getMakers() {
    static std::map < std::string, ObsSpaceFactory * > makers_;
    return makers_;
  }
};

// -----------------------------------------------------------------------------

template<class T>
class ObsSpaceMaker : public ObsSpaceFactory {
  virtual ObsSpaceBase * make(const eckit::Configuration & conf, 
                              const util::DateTime & bgn, const util::DateTime & end)
    { return new T(conf, bgn, end); }
 public:
  explicit ObsSpaceMaker(const std::string & name) : ObsSpaceFactory(name) {}
};

// -----------------------------------------------------------------------------

}  // namespace ioda

#endif  // IODA_OBSSPACEFACTORY_H_
