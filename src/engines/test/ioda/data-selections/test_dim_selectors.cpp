/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// This program tests that Variable read and write selections work as expected for certain engines.

#include <Eigen/Dense>
#include <iostream>
#include <vector>

#include "ioda/Engines/EngineUtils.h"
#include "ioda/Exception.h"
#include "ioda/Group.h"

void test_group_backend_engine(ioda::Group g) {
  // test_data1 is a 4x4 matrix that looks like:
  //  1  2  3  4
  //  5  6  7  8
  //  9 10 11 12
  // 13 14 15 16
  Eigen::ArrayXXi test_data1(4, 4);
  test_data1 << 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16;

  // std::cerr << test_data1 << std::endl << std::endl;

  // Make a variable in the file and write test_data1.
  ioda::Variable file_test_data1 = g.vars.create<int>("test_data1", {4, 4}).writeWithEigenRegular(test_data1);

  // Try selecting by dimension indices
  // overlay_data
  // 17 18 19
  // 20 21 22
  Eigen::ArrayXXi overlay_data(2, 3);
  overlay_data << 17, 18, 19, 20, 21, 22;
  file_test_data1.writeWithEigenRegular(overlay_data,
                                        ioda::Selection()
                                          .extent({2, 3})
                                          .select({ioda::SelectionOperator::SET, 0, {0, 1}})
                                          .select({ioda::SelectionOperator::AND, 1, {0, 1, 2}}),
                                        ioda::Selection()
                                          .select({ioda::SelectionOperator::SET, 0, {0, 2}})
                                          .select({ioda::SelectionOperator::AND, 1, {0, 1, 3}}));

  // test_data1 should look like
  // 17 18  3 19
  //  5  6  7  8
  // 20 21 11 22
  // 13 14 15 16

  // And check our read function.
  Eigen::ArrayXXi reference(4, 4);
  reference << 17, 18, 3, 19, 5, 6, 7, 8, 20, 21, 11, 22, 13, 14, 15, 16;

  Eigen::ArrayXXi check;
  file_test_data1.readWithEigenRegular(check);

  bool r = check.isApprox(reference);
  if (!r)
    throw;  // jedi_throw.add("Reason", "Test 1 result for file_test_data1 do not match expected results");

  // Try selecting along only one of the two dimensions
  // overlay_data
  //  23 24 25 26
  Eigen::ArrayXi overlay2_data(4);
  overlay2_data << 23, 24, 25, 26;
  file_test_data1.writeWithEigenRegular(
    overlay2_data, ioda::Selection().extent({4}).select({ioda::SelectionOperator::SET, 0, {0, 1, 2, 3}}),
    ioda::Selection().select({ioda::SelectionOperator::SET, 1, {3}}));

  // test_data1 should look like
  // 17 18  3 23
  //  5  6  7 24
  // 20 21 11 25
  // 13 14 15 26

  // And check our read function.
  Eigen::ArrayXXi reference2(4, 4);
  reference2 << 17, 18, 3, 23, 5, 6, 7, 24, 20, 21, 11, 25, 13, 14, 15, 26;

  Eigen::ArrayXXi check2;
  file_test_data1.readWithEigenRegular(check2);

  bool r2 = check2.isApprox(reference2);
  if (!r2)
    throw;  // jedi_throw.add("Reason", "Test 2 result for file_test_data1 do not match expected results");
}

int main(int argc, char** argv) {
  using namespace ioda;
  using namespace std;
  try {
    auto f = Engines::constructFromCmdLine(argc, argv, "test-dim-selectors.hdf5");
    test_group_backend_engine(f);
  } catch (const std::exception& e) {
    ioda::unwind_exception_stack(e);
    return 1;
  }
  return 0;
}
