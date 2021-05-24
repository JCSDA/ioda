/*
 * (C) Copyright 2009-2016 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation nor
 * does it submit to any jurisdiction.
 */

#ifndef TEST_IODA_OBSVECTOR_H_
#define TEST_IODA_OBSVECTOR_H_

#include <memory>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#define ECKIT_TESTING_SELF_REGISTER_CASES 0

#include "eckit/config/LocalConfiguration.h"
#include "eckit/testing/Test.h"

#include "oops/mpi/mpi.h"
#include "oops/runs/Test.h"
#include "oops/test/TestEnvironment.h"
#include "oops/util/dot_product.h"
#include "oops/util/Logger.h"

#include "ioda/IodaTrait.h"
#include "ioda/ObsSpace.h"
#include "ioda/ObsVector.h"

namespace ioda {
namespace test {

// -----------------------------------------------------------------------------

class ObsVecTestFixture : private boost::noncopyable {
  typedef ioda::ObsSpace ObsSpace_;

 public:
  static std::vector<boost::shared_ptr<ObsSpace_> > & obspace() {return getInstance().ospaces_;}

  static void cleanup() {
    auto &spaces = getInstance().ospaces_;
    for (auto &space : spaces) {
      space.reset();
    }
  }

 private:
  static ObsVecTestFixture& getInstance() {
    static ObsVecTestFixture theObsVecTestFixture;
    return theObsVecTestFixture;
  }

  ObsVecTestFixture(): ospaces_() {
    util::DateTime bgn((::test::TestEnvironment::config().getString("window begin")));
    util::DateTime end((::test::TestEnvironment::config().getString("window end")));

    std::vector<eckit::LocalConfiguration> conf;
    ::test::TestEnvironment::config().get("observations", conf);

    for (std::size_t jj = 0; jj < conf.size(); ++jj) {
      eckit::LocalConfiguration obsconf(conf[jj], "obs space");
      std::string distname = obsconf.getString("distribution", "RoundRobin");
      boost::shared_ptr<ObsSpace_> tmp(new ObsSpace_(obsconf, oops::mpi::world(),
                                                     bgn, end, oops::mpi::myself()));
      ospaces_.push_back(tmp);
      eckit::LocalConfiguration ObsDataInConf;
      obsconf.get("obsdatain", ObsDataInConf);
    }
  }

  ~ObsVecTestFixture() {}

  std::vector<boost::shared_ptr<ObsSpace_> > ospaces_;
};

// -----------------------------------------------------------------------------

void testConstructor() {
  typedef ObsVecTestFixture Test_;
  typedef ioda::ObsVector  ObsVector_;

  for (std::size_t jj = 0; jj < Test_::obspace().size(); ++jj) {
    std::unique_ptr<ObsVector_> ov(new ObsVector_(*Test_::obspace()[jj]));
    EXPECT(ov.get());

    ov.reset();
    EXPECT(!ov.get());
  }
}

// -----------------------------------------------------------------------------

void testCopyConstructor() {
  typedef ObsVecTestFixture Test_;
  typedef ioda::ObsVector  ObsVector_;

  for (std::size_t jj = 0; jj < Test_::obspace().size(); ++jj) {
    std::unique_ptr<ObsVector_> ov(new ObsVector_(*Test_::obspace()[jj]));

    std::unique_ptr<ObsVector_> other(new ObsVector_(*ov));
    EXPECT(other.get());

    other.reset();
    EXPECT(!other.get());

    EXPECT(ov.get());
  }
}

// -----------------------------------------------------------------------------

void testNotZero() {
  typedef ObsVecTestFixture Test_;
  typedef ioda::ObsVector  ObsVector_;
  const double zero = 0.0;

  for (std::size_t jj = 0; jj < Test_::obspace().size(); ++jj) {
    ObsVector_ ov(*Test_::obspace()[jj]);

    ov.random();

    const double ovov2 = dot_product(ov, ov);
    EXPECT(ovov2 > zero);

    ov.zero();

    const double zz = dot_product(ov, ov);
    EXPECT(zz == zero);
  }
}

// -----------------------------------------------------------------------------

void testRead() {
  typedef ObsVecTestFixture Test_;
  typedef ioda::ObsVector  ObsVector_;

  std::vector<eckit::LocalConfiguration> conf;
  ::test::TestEnvironment::config().get("observations", conf);

  for (std::size_t jj = 0; jj < Test_::obspace().size(); ++jj) {
    ioda::ObsSpace * Odb = Test_::obspace()[jj].get();

    // Grab the expected RMS value and tolerance from the obsdb_ configuration.
    // Grab the obs space and test data configurations
    eckit::LocalConfiguration testConfig;
    conf[jj].get("test data", testConfig);
    double ExpectedRms = testConfig.getDouble("rms ref");
    double Tol = testConfig.getDouble("tolerance");

    // Read in a vector and check contents with norm function.
    std::unique_ptr<ObsVector_> ov(new ObsVector_(*Odb, "ObsValue"));
    double Rms = ov->rms();

    EXPECT(oops::is_close(Rms, ExpectedRms, Tol));
  }
}

// -----------------------------------------------------------------------------

void testSave() {
  typedef ObsVecTestFixture Test_;
  typedef ioda::ObsVector  ObsVector_;

  ioda::ObsSpace * Odb;
  double Rms;
  double ExpectedRms;

  std::vector<eckit::LocalConfiguration> conf;
  ::test::TestEnvironment::config().get("observations", conf);

  for (std::size_t jj = 0; jj < Test_::obspace().size(); ++jj) {
    Odb = Test_::obspace()[jj].get();
    bool read_obs_from_separate_file =
      conf[jj].getBool("obs space.read obs from separate file", false);

    // Read in a vector and save the rms value. Then write the vector into a
    // test group, read it out of the test group and compare the rms of the
    // vector read out of the test group with that of the original.
    std::unique_ptr<ObsVector_> ov_orig(new ObsVector_(*Odb, "ObsValue"));
    ExpectedRms = ov_orig->rms();

    if (!read_obs_from_separate_file)
      ov_orig->save("ObsTest");

    std::unique_ptr<ObsVector_> ov_test(new ObsVector_(*Odb, "ObsTest"));
    Rms = ov_test->rms();

    EXPECT(oops::is_close(Rms, ExpectedRms, 1.0e-12));
  }
}

// -----------------------------------------------------------------------------
/// \brief tests ObsVector::axpy methods
/// \details Tests the following for a random vector vec1:
/// 1. Calling ObsVector::axpy with a single number (2.0) returns the same result
///    as calling it with a vector of 2.0
/// 2. Calling ObsVector::axpy with vectors of coefficients that differ across
///    variables gives reasonable result. axpy is called twice, the coefficients
///    between the two different calls add to 2.0.
void testAxpy() {
  typedef ObsVecTestFixture Test_;
  typedef ioda::ObsVector  ObsVector_;

  for (std::size_t jj = 0; jj < Test_::obspace().size(); ++jj) {
    ioda::ObsSpace & obspace = *Test_::obspace()[jj];
    ObsVector_ vec1(obspace);
    vec1.random();

    // call axpy with coefficient 2.0 two different ways
    ObsVector_ vec2(vec1);
    vec2.axpy(2.0, vec1);
    ObsVector_ vec3(vec1);
    std::vector<double> beta(obspace.obsvariables().size(), 2.0);
    vec3.axpy(beta, vec1);
    oops::Log::test() << "Testing ObsVector::axpy" << std::endl;
    oops::Log::test() << "x = " << vec1 << std::endl;
    oops::Log::test() << "x.axpy(2, x) = " << vec2 << std::endl;
    oops::Log::test() << "x.axpy(vector of 2, x) = " << vec3 << std::endl;
    EXPECT(oops::is_close(vec2.rms(), vec3.rms(), 1.0e-8));

    // call axpy with vector of different values
    std::vector<double> beta1(obspace.obsvariables().size());
    std::vector<double> beta2(obspace.obsvariables().size());
    for (size_t jj = 0; jj < beta1.size(); ++jj) {
      beta1[jj] = static_cast<double>(jj)/beta1.size();
      beta2[jj] = 2.0 - beta1[jj];
    }
    oops::Log::test() << "beta1 = " << beta1 << ", beta2 = " << beta2 << std::endl;
    ObsVector_ vec4(vec1);
    vec4.axpy(beta1, vec1);
    oops::Log::test() << "x.axpy(beta1, x) = " << vec4 << std::endl;
    vec4.axpy(beta2, vec1);
    oops::Log::test() << "x.axpy(beta2, x) = " << vec4 << std::endl;
    EXPECT(oops::is_close(vec4.rms(), vec3.rms(), 1.0e-8));
  }
}

/// \brief tests ObsVector::dot_product methods
/// \details Tests the following for a random vector vec1:
/// 1. Calling ObsVector::dot_product and calling ObsVector::multivar_dot_product
///    are consistent.
void testDotProduct() {
  typedef ObsVecTestFixture Test_;
  typedef ioda::ObsVector  ObsVector_;

  for (std::size_t jj = 0; jj < Test_::obspace().size(); ++jj) {
    ioda::ObsSpace & obspace = *Test_::obspace()[jj];
    ObsVector_ vec1(obspace);
    vec1.random();
    ObsVector_ vec2(obspace);
    vec2.random();

    double dp1 = vec1.dot_product_with(vec2);
    std::vector<double> dp2 = vec1.multivar_dot_product_with(vec2);
    oops::Log::test() << "Testing ObsVector::dot_product" << std::endl;
    oops::Log::test() << "x1 = " << vec1 << std::endl;
    oops::Log::test() << "x2 = " << vec2 << std::endl;
    oops::Log::test() << "x1.dot_product_with(x2) = " << dp1 << std::endl;
    oops::Log::test() << "x1.multivar_dot_product_with(x2) = " << dp2 << std::endl;

    // test that size of vector returned by multivar dot product is correct
    EXPECT_EQUAL(dp2.size(), vec1.nvars());
    // test that dot products are consistent (sum of all elements in multivar one
    // is the same as the scalar one)
    EXPECT(oops::is_close(dp1, std::accumulate(dp2.begin(), dp2.end(), 0.0), 1.0e-12));
  }
}

// -----------------------------------------------------------------------------

void testDistributedMath() {
  typedef ioda::ObsVector  ObsVector_;
  typedef ioda::ObsSpace ObsSpace_;
  typedef std::vector< std::shared_ptr< ObsVector_> > ObsVectors_;

  // Some of the ObsVector math routines require global communications,
  // and so are performed differently for different distributions. But the
  // answers should always be the same regardless of distribution.

  // get the list of distributions to test with
  std::vector<std::string> dist_names =
    ::test::TestEnvironment::config().getStringVector("distributions");
  for (std::size_t ii = 0; ii < dist_names.size(); ++ii) {
    oops::Log::debug() << "using distribution: " << dist_names[ii] << std::endl;
  }

  // Get some config information that is the same regardless of distribution
  util::DateTime bgn((::test::TestEnvironment::config().getString("window begin")));
  util::DateTime end((::test::TestEnvironment::config().getString("window end")));
  std::vector<eckit::LocalConfiguration> conf;
  ::test::TestEnvironment::config().get("observations", conf);

  // for each distribution, create the set of obs vectors
  std::vector< ObsVectors_ > dist_obsvecs;
  std::vector< std::shared_ptr<ObsSpace_> > dist_obsdbs;
  for (std::size_t dd = 0; dd < dist_names.size(); ++dd) {
    ObsVectors_ obsvecs;
    for (std::size_t jj = 0; jj < conf.size(); ++jj) {
       // We want to cycle through the set of distributions that are specified
       // in the list with the keyword "distributions" in the YAML. The test fixture
       // has already constructed all of the obs spaces listed in the YAML, and we
       // are repeating that action inside this loop. In other words, we are doubling
       // up the obs space objects that a specified in the YAML.
       //
       // This is okay unless the YAML has specified an output file anywhere. The issue
       // is that the output file is written during the destructor and the HDF library
       // (unfortunately) tends to keep file descriptors open until the process terminates.
       // Therefore it is possible for the file writes to collide, causing the test to
       // crash, if the obs space created here is writing to the same file as the
       // corresponding obs space in the test fixture.
       //
       // The fix is to tag on the name of the distribution on the output file name here
       // to prevent the collision. The collision avoidance is absolutely guaranteed, but
       // we can do it in a way that is unlikely to collide with any other output file
       // names in the YAML. Note that this also prevents clobbering any output files
       // specfied in the YAML.
       eckit::LocalConfiguration obsconf(conf[jj], "obs space");
       obsconf.set("distribution", dist_names[dd]);
       if (obsconf.has("obsdataout.obsfile")) {
           std::string fileName = obsconf.getString("obsdataout.obsfile");
           std::string fileTag = std::string("_Dist_") + dist_names[dd];
           std::size_t pos = fileName.find_last_of(".");
           if (pos != std::string::npos) {
               // have a suffix on the file name, insert tag before the suffix
               fileName.insert(pos, fileTag);
           } else {
               // do not have a suffix on the file name, append tag to end of file name
               fileName += fileTag;
           }
           obsconf.set("obsdataout.obsfile", fileName);
       }

       // Instantiate the obs space with the distribution we are testing
       std::shared_ptr<ObsSpace_> obsdb(new ObsSpace(obsconf, oops::mpi::world(),
                                                     bgn, end, oops::mpi::myself()));
       std::shared_ptr<ObsVector_> obsvec(new ObsVector_(*obsdb, "ObsValue"));
       oops::Log::debug() << dist_names[dd] << ": " << *obsvec << std::endl;
       dist_obsdbs.push_back(obsdb);
       obsvecs.push_back(obsvec);
    }
    dist_obsvecs.push_back(obsvecs);
  }

  // For each ObsVector make sure the math is the same regardless of distribution.
  // Test rms(), nobs(), dot_product_with()
  for (std::size_t ii = 0; ii < conf.size(); ++ii) {
    // get the values for the first distribution
    int nobs = dist_obsvecs[0][ii]->nobs();
    double rms = dist_obsvecs[0][ii]->rms();
    double dot = dist_obsvecs[0][ii]->dot_product_with(*dist_obsvecs[0][ii]);

    // make sure the values are the same for all the other distributions
    for (std::size_t jj = 1; jj < dist_obsvecs.size(); ++jj) {
      int nobs2 = dist_obsvecs[jj][ii]->nobs();
      double rms2 = dist_obsvecs[jj][ii]->rms();
      double dot2 = dist_obsvecs[jj][ii]->dot_product_with(*dist_obsvecs[jj][ii]);

      EXPECT(nobs == nobs2);
      EXPECT(oops::is_close(rms, rms2, 1.0e-12));
      EXPECT(oops::is_close(dot, dot2, 1.0e-12));
    }
  }
}

// -----------------------------------------------------------------------------

void testCleanup() {
  // This test removes the obsspaces from the test fixture and ensures that they evict
  // their contents to disk successfully. The saveToFile logic runs in a destructor,
  // so this test ensures that we are not executing features that we want to test after
  // the eckit testing environment reports success.
  typedef ObsVecTestFixture Test_;

  Test_::cleanup();
}

// -----------------------------------------------------------------------------

class ObsVector : public oops::Test {
 public:
  ObsVector() {}
  virtual ~ObsVector() {}

 private:
  std::string testid() const override {return "test::ObsVector<ioda::IodaTrait>";}

  void register_tests() const override {
    std::vector<eckit::testing::Test>& ts = eckit::testing::specification();

     ts.emplace_back(CASE("ioda/ObsVector/testConstructor")
      { testConstructor(); });
     ts.emplace_back(CASE("ioda/ObsVector/testCopyConstructor")
      { testCopyConstructor(); });
     ts.emplace_back(CASE("ioda/ObsVector/testNotZero")
      { testNotZero(); });
     ts.emplace_back(CASE("ioda/ObsVector/testRead")
      { testRead(); });
     ts.emplace_back(CASE("ioda/ObsVector/testSave")
      { testSave(); });
     ts.emplace_back(CASE("ioda/ObsVector/testAxpy")
      { testAxpy(); });
     ts.emplace_back(CASE("ioda/ObsVector/testDotProduct")
      { testDotProduct(); });
     ts.emplace_back(CASE("ioda/ObsVector/testDistributedMath")
      { testDistributedMath(); });
     ts.emplace_back(CASE("ioda/ObsVector/testCleanup")
      { testCleanup(); });
  }

  void clear() const override {}
};

// =============================================================================

}  // namespace test
}  // namespace ioda

#endif  // TEST_IODA_OBSVECTOR_H_
