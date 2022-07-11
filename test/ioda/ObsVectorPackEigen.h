/*
 * (C) Copyright 2021- UCAR.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef TEST_IODA_OBSVECTORPACKEIGEN_H_
#define TEST_IODA_OBSVECTORPACKEIGEN_H_

#include <Eigen/Dense>

#include <string>
#include <vector>

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include "eckit/config/LocalConfiguration.h"
#include "eckit/testing/Test.h"

#include "oops/mpi/mpi.h"
#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"
#include "oops/util/Logger.h"

#include "ioda/ObsDataVector.h"
#include "ioda/ObsSpace.h"
#include "ioda/ObsVector.h"

namespace ioda {
namespace test {

// -----------------------------------------------------------------------------

/// \brief tests ObsVector::packEigen, packEigenSize methods and mask methods
/// \details Tests that:
/// - number of local masked obs returned by ObsVector::packEigenSize is the same
///   as reference in yaml (reference local masked nobs);
/// - norm of Eigen::VectorXd returned by ObsVector::packEigen is close to the
///   reference specified in yaml (reference local masked norm);
/// - norm of a random vector with mask applied is different from the same vector
///   before mask application;
/// - norm of a random vector with mask(ObsDataVector<int>) applied is the same as
///   norm of the same vector with mask(ObsVector).
void testPackEigen() {
  std::vector<eckit::LocalConfiguration> conf;
  ::test::TestEnvironment::config().get("observations", conf);
  util::DateTime bgn((::test::TestEnvironment::config().getString("window begin")));
  util::DateTime end((::test::TestEnvironment::config().getString("window end")));

  for (std::size_t jj = 0; jj < conf.size(); ++jj) {
     eckit::LocalConfiguration obsconf(conf[jj], "obs space");
     ioda::ObsTopLevelParameters obsparams;
     obsparams.validateAndDeserialize(obsconf);
     ioda::ObsSpace obsdb(obsparams, oops::mpi::world(), bgn, end, oops::mpi::myself());

     const size_t rank = obsdb.distribution()->rank();
     ioda::ObsVector obsvec(obsdb, "ObsValue");

     // test packEigenSize
     const std::string maskname = conf[jj].getString("mask variable");
     ioda::ObsDataVector<int> mask(obsdb, obsdb.assimvariables(), maskname);
     ioda::ObsVector maskvector(obsdb);
     maskvector.mask(mask);
     const size_t size = obsvec.packEigenSize(maskvector);
     const std::vector<size_t> ref_sizes =
                       conf[jj].getUnsignedVector("reference local masked nobs");
     EXPECT_EQUAL(size, ref_sizes[rank]);

     // test packEigen
     Eigen::VectorXd packed = obsvec.packEigen(maskvector);
     const std::vector<double> ref_norms =
                       conf[jj].getDoubleVector("reference local masked norm");
     EXPECT(oops::is_close(packed.norm(), ref_norms[rank], 1.e-5));

     // test mask
     ioda::ObsVector vec1(obsdb);
     vec1.random();
     ioda::ObsVector vec2 = vec1;
     vec1.mask(mask);
     EXPECT_NOT_EQUAL(vec1.rms(), vec2.rms());
     vec2.mask(maskvector);
     EXPECT_EQUAL(vec1.rms(), vec2.rms());
  }
}

// -----------------------------------------------------------------------------

class ObsVectorPackEigen : public oops::Test {
 public:
  ObsVectorPackEigen() = default;
  virtual ~ObsVectorPackEigen() = default;

 private:
  std::string testid() const override {return "test::ObsVector<ioda::IodaTrait>";}

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

     ts.emplace_back(CASE("ioda/ObsVectorPackEigen/testPackEigen")
      { testPackEigen(); });
  }

  void clear() const override {}
};

// =============================================================================

}  // namespace test
}  // namespace ioda

#endif  // TEST_IODA_OBSVECTORPACKEIGEN_H_
