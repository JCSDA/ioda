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

  // overlay_data1 is a 2x2 matrix:
  // 17 18
  // 19 20
  Eigen::ArrayXXi overlay_data1(2, 2);
  overlay_data1 << 17, 18, 19, 20;

  // std::cerr << overlay_data1 << std::endl << std::endl;

  // Write the 2x2 overlay on top of the existing data.

  // We use two selectors. The first selects the data in memory (i.e. the inputs that we
  // provide). The second selects the range of the data in ioda.
  // Because we are not writing the full dimensions of the variable, we need
  // to specify selectors for both user memory and ioda.

  // The memory selector defines a hyperslab, starting at (0,0), with size (2,2).
  //   Since the 2x2 in-memory object has different dimensions than the object stored in
  //   ioda (4x4), we need to pass along the dimensions of our object to properly write
  //   rows and columns. This is accomplished by setting extent({2,2}).
  // The ioda 'file' selector also defines a hyperslab, but it starts at (2,2) with size (2,2).
  //   So, we are writing the lower right quadrant of the matrix.
  file_test_data1.writeWithEigenRegular(
    overlay_data1, ioda::Selection().extent({2, 2}).select({ioda::SelectionOperator::SET, {0, 0}, {2, 2}}),
    ioda::Selection().select({ioda::SelectionOperator::SET, {2, 2}, {2, 2}}));
  // file_test_data1 should now look like:
  //  1  2  3  4
  //  5  6  7  8
  //  9 10 17 18
  // 13 14 19 20

  // Now, let's write this 2x2 object, again, to the same output.
  file_test_data1.writeWithEigenRegular(
    overlay_data1, ioda::Selection().extent({2, 2}).select({ioda::SelectionOperator::SET, {0, 0}, {2, 2}}),
    ioda::Selection().select({ioda::SelectionOperator::SET, {1, 1}, {2, 2}}));
  // file_test_data1 should now look like:
  //  1  2  3  4
  //  5 17 18  8
  //  9 19 20 18
  // 13 14 19 20

  // Let's use a different type of selector: the point selector.
  // We will set (3,0) = 21, and (0,3) = 22.
  std::vector<int> point_vals{21, 22};
  file_test_data1.write(gsl::make_span(point_vals),  // Values of the two points
                        ioda::Selection().extent({2, 1}).select(
                          {ioda::SelectionOperator::SET, {{0, 0}, {1, 0}}}),  // Writing two points
                        ioda::Selection().select({ioda::SelectionOperator::SET,
                                                  {{3, 0}, {0, 3}}}));  // Location where we write the data.
  // file_test_data1 should now look like:
  //  1  2  3 22
  //  5 17 18  8
  //  9 19 20 18
  // 21 14 19 20

  // And check our read function.
  Eigen::ArrayXXi reference(4, 4);
  reference << 1, 2, 3, 22, 5, 17, 18, 8, 9, 19, 20, 18, 21, 14, 19, 20;

  Eigen::ArrayXXi check;
  file_test_data1.readWithEigenRegular(check);

  bool r = check.isApprox(reference);
  if (!r) throw std::logic_error("Test failure.");
}

int main(int argc, char** argv) {
  using namespace ioda;
  using namespace std;
  try {
    auto f = Engines::constructFromCmdLine(argc, argv, "test-data-selections.hdf5");
    test_group_backend_engine(f);
  } catch (const std::exception& e) {
    ioda::unwind_exception_stack(e);
    return 1;
  }
  return 0;
}
