/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include <cmath>
#include <iostream>
#include <vector>

#include "ioda/Engines/EngineUtils.h"
#include "ioda/Exception.h"
#include "ioda/Group.h"

// These tests really need a better check system.
// Boost unit tests would have been excellent here.

// Check dimensions against expected values
void check_dimensions(const std::string& name, const ioda::Dimensions& dims,
                      const std::vector<ioda::Dimensions_t>& exp_dims) {
  std::string err_msg;
  std::string exp_msg;
  std::string res_msg;
  // Check rank of dimensions
  if (dims.dimensionality != gsl::narrow<ioda::Dimensions_t>(exp_dims.size())) {
    err_msg = name + std::string(": dimensionality not equal to expected value");
    exp_msg = std::string("  expected dimensionality");
    res_msg = std::string("  ") + name + std::string(": dimensionality");
    throw ioda::Exception(err_msg.c_str(), ioda_Here())
      .add(exp_msg, exp_dims.size())
      .add(res_msg, dims.dimensionality);
  }

  // Check dimension sizes
  for (std::size_t i = 0; i < exp_dims.size(); ++i) {
    if (dims.dimsCur[i] != exp_dims[i]) {
      err_msg =
        name + std::string(": dimension ") + std::to_string(i) + std::string(" not equal to expected value");
      exp_msg = std::string("  expected dimsCur[") + std::to_string(i) + std::string("]");
      res_msg = std::string("  ") + name + std::string(": dimsCur[") + std::to_string(i) + std::string("]");
      throw ioda::Exception(err_msg.c_str(), ioda_Here())
        .add(exp_msg, exp_dims[i])
        .add(res_msg, dims.dimsCur[i]);
    }
  }
}

// Check double data against expected values
void check_data(const std::string& name, const std::vector<double>& data,
                const std::vector<double>& exp_data) {
  std::string err_msg;
  std::string exp_msg;
  std::string res_msg;
  // Check size of data
  if (data.size() != exp_data.size()) {
    err_msg = name + std::string(": data size not equal to expected value");
    exp_msg = std::string("  expected size");
    res_msg = std::string("  ") + name + std::string(": size");
    throw ioda::Exception(err_msg.c_str(), ioda_Here())
      .add(exp_msg, exp_data.size())
      .add(res_msg, data.size());
  }

  // Check data values
  for (std::size_t i = 0; i < exp_data.size(); ++i) {
    double check = fabs((data[i] / exp_data[i]) - 1.0);
    if (check > 1.0e-3) {
      err_msg = name + std::string(": element ") + std::to_string(i) +
                std::string(" not within tolerence (1e-3) of expected value");
      exp_msg = std::string("  expected data[") + std::to_string(i) + std::string("]");
      res_msg = std::string("  ") + name + std::string(": data[") + std::to_string(i) + std::string("]");
      throw ioda::Exception(err_msg.c_str(), ioda_Here())
        .add(exp_msg, exp_data[i])
        .add(res_msg, data[i]);
    }
  }
}

// Check int data against expected values
void check_data(const std::string& name, const std::vector<int>& data, const std::vector<int>& exp_data) {
  std::string err_msg;
  std::string exp_msg;
  std::string res_msg;
  // Check size of data
  if (data.size() != exp_data.size()) {
    err_msg = name + std::string(": data size not equal to expected value");
    exp_msg = std::string("  expected size");
    res_msg = std::string("  ") + name + std::string(": size");
    throw ioda::Exception(err_msg.c_str(), ioda_Here())
      .add(exp_msg, exp_data.size())
      .add(res_msg, data.size());
  }

  // Check data values
  for (std::size_t i = 0; i < exp_data.size(); ++i) {
    if (data[i] != exp_data[i]) {
      err_msg =
        name + std::string(": element ") + std::to_string(i) + std::string(" not equal to expected value");
      exp_msg = std::string("  expected data[") + std::to_string(i) + std::string("]");
      res_msg = std::string("  ") + name + std::string(": data[") + std::to_string(i) + std::string("]");
      throw ioda::Exception(err_msg.c_str(), ioda_Here())
        .add(exp_msg, exp_data[i])
        .add(res_msg, data[i]);
    }
  }
}

// Be sure to keep this routine and check_group_structure in sync.
void build_group_structure(ioda::Group g) {
  // Create some sub groups
  auto g_c1 = g.create("Child1");
  auto g_c2 = g.create("Child2");

  // Place attributes in the sub groups
  g_c1.atts.template add<double>("double_single", {3.14159}, {1});

  g_c2.atts.template add<int>("int_2x2", {1, 2, 3, 4}, {2, 2});

  // Place variables in the sub groups
  ioda::VariableCreationParameters params_1;
  params_1.setFillValue<double>(-999);
  params_1.chunk = true;
  params_1.compressWithGZIP();
  auto v_double = g_c1.vars.template create<double>("double", {2, 2}, {2, 2}, params_1);
  v_double.write<double>({10.0, 11.0, 12.0, 13.0});
  v_double.atts.template add<int>("int_2x3", {-2, -1, 0, 1, 2, 3}, {2, 3});
}

// Be sure to keep this routine and build_group_structure in sync.
void check_group_structure(ioda::Group g) {
  std::vector<ioda::Dimensions_t> exp_dims;
  std::vector<double> exp_data_d;
  std::vector<int> exp_data_i;

  // Verify the sub groups. The open function will throw an exception
  // if the sub group does not exist.
  auto g_c1 = g.open("Child1");
  auto g_c2 = g.open("Child2");

  // Check the sub group attributes
  auto attr = g_c1.atts.open("double_single");
  ioda::Dimensions a_dims = attr.getDimensions();
  exp_dims = {1};
  check_dimensions("group attribute: double_single", a_dims, exp_dims);
  std::vector<double> a_data_d;
  attr.read(a_data_d);
  exp_data_d = {3.14159};
  check_data("group attribute: double_single", a_data_d, exp_data_d);

  attr = g_c2.atts.open("int_2x2");
  a_dims = attr.getDimensions();
  exp_dims = {2, 2};
  check_dimensions("group attribute: int_2x2", a_dims, exp_dims);
  std::vector<int> a_data_i;
  attr.read(a_data_i);
  exp_data_i = {1, 2, 3, 4};
  check_data("group attribute: int_2x2", a_data_i, exp_data_i);

  // Check the sub group variable
  auto var = g_c1.vars.open("double");
  ioda::Dimensions v_dims = var.getDimensions();
  exp_dims = {2, 2};
  check_dimensions("varable: double", v_dims, exp_dims);
  std::vector<double> v_data_d;
  var.read<double>(v_data_d);
  exp_data_d = {10.0, 11.0, 12.0, 13.0};
  check_data("variable: double", v_data_d, exp_data_d);

  attr = var.atts.open("int_2x3");
  a_dims = attr.getDimensions();
  exp_dims = {2, 3};
  check_dimensions("variable attribute: int_2x3", a_dims, exp_dims);
  attr.read(a_data_i);
  exp_data_i = {-2, -1, 0, 1, 2, 3};
  check_data("variable attribute: int_2x3", a_data_i, exp_data_i);
}

int main(int argc, char** argv) {
  using namespace ioda;
  using namespace std;
  try {
    auto f = Engines::constructFromCmdLine(argc, argv, "test-persist.hdf5");

    // Build sub-groups containing variables and attributes
    // in one function. Then check their contents in another
    // function call. Do this to make sure that the
    // group/attribtute/variable structure persists.
    build_group_structure(f);
    check_group_structure(f);

  } catch (const std::exception& e) {
    ioda::unwind_exception_stack(e);
    return 1;
  }
  return 0;
}
