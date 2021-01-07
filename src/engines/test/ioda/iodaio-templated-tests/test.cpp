/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include <Eigen/Dense>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "ioda/Engines/Factory.h"
#include "ioda/Group.h"

// These tests really need a better check system.
// Boost unit tests would have been excellent here.

template <class E, typename Container>
void test_eigen_regular_attributes(Container g, const E& eigen_data, bool is2D = true) {
  g.atts.addWithEigenRegular("data", eigen_data, is2D);

  // check_data can't be purely of type E, here, since we might be
  // feeding in a map or a block.
  typedef typename E::Scalar ScalarType;
  Eigen::Array<ScalarType, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> check_data;
  g.atts.readWithEigenRegular("data", check_data);

  // Check dimensions, then check values.
  if (is2D) {
    if (eigen_data.rows() != check_data.rows()) throw;
    if (eigen_data.cols() != check_data.cols()) throw;
    for (Eigen::Index i = 0; i < eigen_data.rows(); ++i)
      for (Eigen::Index j = 0; j < eigen_data.cols(); ++j)
        if (eigen_data(i, j) != check_data(i, j)) throw;
  } else {
    // Data are read as a column vector. No real way to get
    // around this, since we are storing only a 1-D object.
    if (eigen_data.size() != check_data.size()) throw;
    if (eigen_data.rows() != check_data.rows()) check_data.transposeInPlace();
    for (Eigen::Index i = 0; i < eigen_data.rows(); ++i)
      for (Eigen::Index j = 0; j < eigen_data.cols(); ++j)
        if (eigen_data(i, j) != check_data(i, j)) throw;
  }
}

template <class E, typename Container>
void test_eigen_tensor_attributes(Container g, const E& eigen_data) {
  g.atts.addWithEigenTensor("data", eigen_data);

  // TODO(rhoneyager): change to make a tensor from whatever
  // the input type is.
  E check_data(eigen_data.dimensions());
  g.atts.readWithEigenTensor("data", check_data);

  const ioda::Dimensions dims = ioda::detail::EigenCompat::getTensorDimensions(eigen_data);

  for (ioda::Dimensions_t i = 0; i < dims.numElements; ++i)
    if (eigen_data.data()[i] != check_data.data()[i]) throw;
}

template <typename T>
bool test_equal(const T& a, const T& b) {
  return a == b;
}

template <>
bool test_equal(const float& a, const float& b) {
  return std::abs(a - b) < 0.00001f;
}

template <>
bool test_equal(const double& a, const double& b) {
  return std::abs(a - b) < 0.00001f;
}

template <>
bool test_equal(const long double& a, const long double& b) {
  return std::abs(a - b) < 0.00001f;
}

template <typename T, typename Container>
void test_attribute_functions(Container g, std::initializer_list<T> values,
                              std::initializer_list<ioda::Dimensions_t> dimensions) {
  // Add attribute using initializer lists
  g.atts.template add<T>("initializer_lists", values, dimensions);

  // Add attribute using gsl spans
  using namespace std;
  vector<T> vals{values};
  g.atts.template add<T>("gsl_spans", vals, dimensions);

  // Open an attribute
  ioda::Attribute a_spans = g.atts["gsl_spans"];
  ioda::Attribute a_ilist = g.atts.open("initializer_lists");

  // Verify dimensionality
  auto adims = a_spans.getDimensions();
  if (adims.dimensionality != gsl::narrow<ioda::Dimensions_t>(dimensions.size())) throw;
  // Verify dimensions
  vector<ioda::Dimensions_t> dims{dimensions};
  size_t numElems = (dims.empty()) ? 0 : 1;
  for (size_t i = 0; i < dims.size(); ++i) {
    if (dims[i] != adims.dimsCur[i]) throw;
    if (dims[i] != adims.dimsMax[i]) throw;
    numElems *= dims[i];
  }
  if (gsl::narrow<ioda::Dimensions_t>(numElems) != adims.numElements) throw;

  // Read the attribute and check values
  vector<T> v_at1;
  vector<T> v_at1_presized(numElems);
  a_ilist.read(v_at1);
  // int32_t c_arr_at1[6];
  // at1.read(gsl::make_span(c_arr_at1, 6));

  a_ilist.read(gsl::make_span(v_at1_presized));

  if (v_at1.size() != numElems) throw;

  for (size_t i = 0; i < numElems; ++i) {
    if (!test_equal(v_at1[i], vals[i])) throw;
    if (!test_equal(v_at1_presized[i], vals[i])) throw;
  }

  // TODO(rhoneyager): Verify data type.
}

template <class E, typename Container>
void test_eigen_regular_variable(Container g, const E& eigen_data) {
  typedef typename E::Scalar ScalarType;
  // g.vars.create("data", )
  ioda::VariableCreationParameters params_1;
  params_1.setFillValue<ScalarType>(0);
  params_1.chunk = true;
  params_1.compressWithGZIP();

  auto v_double =
    g.vars.template create<ScalarType>("var", {eigen_data.rows(), eigen_data.cols()}, {}, params_1);
  v_double.writeWithEigenRegular(eigen_data);

  /*
  std::vector<double> check_v_double;
  v_double.read<double>(check_v_double);
  if (check_v_double.size() != 4) throw;
  if (check_v_double[0] > 9.9 || check_v_double[0] < 9.7) throw;
  if (check_v_double[1] > 9.9 || check_v_double[1] < 9.7) throw;
  if (check_v_double[2] > 2.3 || check_v_double[2] < 2.1) throw;
  if (check_v_double[3] > 1.7 || check_v_double[3] < 1.5) throw;
  */
}

template <class E, typename Container>
void test_eigen_tensor_variable(Container g, const E& eigen_data) {
  typedef typename E::Scalar ScalarType;
  const ioda::Dimensions dims = ioda::detail::EigenCompat::getTensorDimensions(eigen_data);
  auto v = g.vars.template create<ScalarType>("data", dims.dimsCur, dims.dimsMax);
  v.writeWithEigenTensor(eigen_data);

  // TODO(rhoneyager): change to make a tensor from whatever
  // the input type is.
  E check_data(eigen_data.dimensions());
  v.readWithEigenTensor(check_data);

  for (ioda::Dimensions_t i = 0; i < dims.numElements; ++i)
    if (eigen_data.data()[i] != check_data.data()[i]) throw;
}

/// Run a series of tests on the input group.
void test_group_backend_engine(ioda::Group g) {
  // Can we make child groups?
  // Can we nest groups?
  g.create("Test_group_1");
  auto g2 = g.create("Test_group_2");
  g2.create("Child 1");

  // Can we check for group existence?
  if (!g.exists("Test_group_1")) throw;
  // Can we verify that nested groups exist?
  if (!g.exists("Test_group_2/Child 1")) throw;

  // Can we list groups?
  auto g_list = g.list();
  if (g_list.size() != 2) throw;
  // Can we list another way?
  auto g_list2 = g.listObjects();
  if (g_list2.empty()) throw;

  // Can we open groups?
  auto g3 = g2.open("Child 1");
  // Can we open nested groups?
  auto g4 = g.open("Test_group_2/Child 1");

  // Attribute tests
  auto gatt = g.create("Attribute Tests");

  // Let's template these tests and run them in separate groups to prevent name clashes.
  test_attribute_functions<double>(gatt.create("double_single"), {3.14159}, {1});
  test_attribute_functions<double>(gatt.create("double_vector"), {0.1, 0.2, 0.3, 0.4}, {4});
  test_attribute_functions<double>(gatt.create("double_array_2x3"), {1.2, 2.4, 3.6, 4.8, 5.9, 6.3}, {2, 3});
  test_attribute_functions<float>(gatt.create("float_array_2x3"), {1.1f, 2.2f, 3.3f, 4.4f, 5.5f, 6.6f},
                                  {2, 3});
  // test_attribute_functions<int8_t>(gatt.create("int8_t_3x2"), { -1, 1, -2, 2, -3, 3 }, { 3,2 });
  // test_attribute_functions<uint8_t>(gatt.create("uint8_t_3x2"), { 1, 1, 2, 2, 3, 3 }, { 3,2 });
  test_attribute_functions<int16_t>(gatt.create("int16_t_2x2"), {1, -4, 9, -16}, {2, 2});
  test_attribute_functions<uint16_t>(gatt.create("uint16_t_2x2"), {1, 4, 9, 16}, {2, 2});
  test_attribute_functions<int32_t>(gatt.create("int32_t_2x2"), {1, -4, 9, -16}, {2, 2});
  test_attribute_functions<uint32_t>(gatt.create("uint32_t_2x2"), {1, 4, 9, 16}, {2, 2});
  test_attribute_functions<int64_t>(gatt.create("int64_t_2"), {32768, -131072}, {2});
  test_attribute_functions<uint64_t>(gatt.create("uint64_t_2"), {1073741824, 1099511627776}, {2});
  test_attribute_functions<long double>(gatt.create("ld_1"), {1}, {1});
  test_attribute_functions<unsigned long>(gatt.create("ul_1"), {1}, {1});
  test_attribute_functions<size_t>(gatt.create("size_t"), {1}, {1});
  // test_attribute_functions<wchar_t>(gatt.create("wchar_t"), {L'A'}, {1});
  // test_attribute_functions<char16_t>(gatt.create("char16_t"), {u'A'}, {1});
  // test_attribute_functions<char32_t>(gatt.create("char32_t"), {U'A'}, {1});
  // test_attribute_functions<signed char>(gatt.create("schar_t"), {'a'}, {1});
  // test_attribute_functions<unsigned char>(gatt.create("uchar_t"), {'a'}, {1});
  test_attribute_functions<char>(gatt.create("char_t"), {'a'}, {1});

  test_attribute_functions<std::string>(gatt.create("string_t"), {"Hi Steve!", "This is a test."}, {2});
  // BUG: char gets auto-detected as an 8-bit integer type. Need to re-introduce the type-overriding options
  //   when creating, reading and writing attributes.
  // test_attribute_functions<char>(gatt.create("char_t_2"), { 'a', 'b' }, { 2 });
  // BUG: Can't make a span of bools!
  // test_attribute_functions<bool>(gatt.create("bool_t_2"), { true, false }, { 2 });

  Eigen::ArrayXXi int_array_1(3, 3);
  int_array_1 << 1, 2, 3, 4, 5, 6, 7, 8, 9;
  test_eigen_regular_attributes(gatt.create("eigen_ints_3x3"), int_array_1);
  test_eigen_regular_attributes(gatt.create("eigen_ints_2x3"), int_array_1.block(1, 0, 2, 3));
  test_eigen_regular_attributes(gatt.create("eigen_ints_1x3_2D"), int_array_1.block(2, 0, 1, 3));
  test_eigen_regular_attributes(gatt.create("eigen_ints_1x3_scalar"), int_array_1.block(2, 0, 1, 3), false);

  Eigen::Tensor<int, 3> int_tensor_1(2, 3, 4);  // 24 elements in a 2x3x4 tensor.
  for (int i = 0; i < 2; ++i) {
    for (int j = 0; j < 3; ++j) {
      for (int k = 0; k < 4; ++k) {
        int_tensor_1(i, j, k) = (3 * i) + (2 * j) + k;
      }
    }
  }
  test_eigen_tensor_attributes(gatt.create("eigen_tensor_ints_2x3x4"), int_tensor_1);

  // Now let's create some variables.
  // We can also populate variables with data.

  ioda::VariableCreationParameters params_1;
  params_1.setFillValue<double>(-999);
  params_1.chunk = true;
  params_1.compressWithGZIP();
  auto gvar = g.create("Variable Tests");
  auto v_double = gvar.vars.create<double>("Double_var", {2, 2}, {2, 2}, params_1);
  v_double.write<double>({9.8, 9.8, 2.2, 1.6});

  test_attribute_functions<int16_t>(v_double, {1, -1, 2, 4}, {2, 2});

  std::vector<double> check_v_double;
  v_double.read<double>(check_v_double);
  if (check_v_double.size() != 4) throw;
  if (check_v_double[0] > 9.9 || check_v_double[0] < 9.7) throw;
  if (check_v_double[1] > 9.9 || check_v_double[1] < 9.7) throw;
  if (check_v_double[2] > 2.3 || check_v_double[2] < 2.1) throw;
  if (check_v_double[3] > 1.7 || check_v_double[3] < 1.5) throw;

  // Resizable variable tests
  ioda::VariableCreationParameters params_2;
  params_2.setFillValue<double>(-999);
  params_2.chunk = true;
  params_2.chunks = {30};
  auto v_d2 = gvar.vars.create<double>("d2_var", {30}, {90}, params_2);
  v_d2.write<double>({1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
                      16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30});
  v_d2.resize({60});

  // Eigen tests

  Eigen::MatrixXi mat_int_4x4(4, 4);
  mat_int_4x4 << 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16;
  test_eigen_regular_variable(gvar.create("eigen_matrix_ints_4x4"), mat_int_4x4);

  test_eigen_tensor_variable(gvar.create("eigen_tensor_ints"), int_tensor_1);

  // Dimension scale tests
  auto dim_1 = gvar.vars.create<int>("dim_1", {1}, {1});
  dim_1.setIsDimensionScale("dim_1");
  auto var_a = gvar.vars.create<int>("var_a_dim_1", {1}, {1});
  var_a.attachDimensionScale(0, dim_1);
  if (!var_a.isDimensionScaleAttached(0, dim_1)) throw;
  var_a.detachDimensionScale(0, dim_1);
  if (var_a.isDimensionScaleAttached(0, dim_1)) throw;
}

int main(int argc, char** argv) {
  using namespace ioda;
  using namespace std;
  try {
    auto f = Engines::constructFromCmdLine(argc, argv, "test-tempates.hdf5");

    test_group_backend_engine(f);

  } catch (const std::exception& e) {
    cerr << e.what() << endl;
    return 1;
  }
  return 0;
}
