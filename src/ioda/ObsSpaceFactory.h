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

namespace ioda {

// Obs Space Factory
class ObsSpaceFactory {
 public:
  static oops::ObsSpaceBase * create(const eckit::Configuration &,
                               const util::DateTime &, const util::DateTime &);
  virtual ~ObsSpaceFactory() { getMakers().clear(); }
 protected:
  explicit ObsSpaceFactory(const std::string &);
 private:
  virtual oops::ObsSpaceBase * make(const eckit::Configuration &,
                              const util::DateTime &, const util::DateTime &) = 0;
  static std::map < std::string, ObsSpaceFactory * > & getMakers() {
    static std::map < std::string, ObsSpaceFactory * > makers_;
    return makers_;
  }
};

// -----------------------------------------------------------------------------

template<class T>
class ObsSpaceMaker : public ObsSpaceFactory {
  virtual oops::ObsSpaceBase * make(const eckit::Configuration & conf, 
                              const util::DateTime & bgn, const util::DateTime & end)
    { return new T(conf, bgn, end); }
 public:
  explicit ObsSpaceMaker(const std::string & name) : ObsSpaceFactory(name) {}
};

// -----------------------------------------------------------------------------

}  // namespace ioda

#endif  // IODA_OBSSPACEFACTORY_H_
