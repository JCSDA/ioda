/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*!
 * \ingroup ioda_cxx_ex_math
 *
 * @{
 * 
 * \file basic_math.cpp
 * \details
 * This example shows how to use basic math expressions in your code.
 * Unit-aware and missing-value-aware math is implemented through a wrapper around the
 * functions and classes provided by the well-known and well-documented Eigen
 * library <https://eigen.tuxfamily.org/>.
 **/

#include <Eigen/Core>
#include <Eigen/Dense>
#include <exception>
#include <iostream>
#include <vector>

#include "ioda/Exception.h"
#include "ioda/MathOps.h"
#include "ioda/Units.h"

int main(int, char **) {
  using std::cerr;
  using std::cout;
  using std::endl;
  using namespace Eigen;
  using namespace ioda;
  using namespace ioda::udunits;
  try {
    // This tutorial assumes that you are at least somewhat familiar with the Eigen library.
    // If not, take a look at the tutorials here to get started:
    // https://eigen.tuxfamily.org/dox/GettingStarted.html
    // https://eigen.tuxfamily.org/dox/group__DenseMatrixManipulation__chapter.html

    // IODA provides a facility for wrapping any Eigen object for use in unit-aware and
    // missing value-aware math. This is in "ioda/MathOps.h".

    // ------------------------------------------------
    // Basic math

    // Let's start by defining a few 4x3 arrays.
    ArrayXXf a(4, 3), b(4, 3);
    a << 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12;
    b << 1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23;

    cout << "Array 'a' is:\n" << a << endl << endl;
    cout << "Array 'b' is:\n" << b << endl << endl;

    // Converting these arrays into ioda::Math is simple.
    // All math objects are instances of the EigenMath<> template.
    //
    // Use the ToEigenMath function to construct one of these objects. The
    //   ToEigenMath function is easier to call than EigenMath<Derived> since
    //   it performs automatic template overloading.
    // The ToEigenMath function takes the following parameters:
    // 1 - The Eigen object to be encalsulated, in this case an array.
    //     ToEigenMath holds an Eigen object, but only Eigen Arrays and
    //     Matrices "own" a separate copy of their data. Eigen Maps, Refs,
    //     and unevaluated expressions hold references to data.
    //     A move constructor is available to transfer ownership of
    //     an Eigen array, if needed.
    // 2 - The units of the array. Units() or Units("1") denotes no units.
    //     Consult the "units" tutorial to see how you can specify units.
    // 3 - The number used to denote a missing value.
    //     You can use oops::missing as a good default.
    auto A = ToEigenMath(a, Units(), -1000);                // Make a copy of Array 'a'.
    auto B = ToEigenMath(std::move(b), Units("1"), -1000);  // Move Array 'b' into 'B'.

    // To print the arrays:
    cout << "Array 'A' is:\n" << A << endl << endl;
    cout << "Array 'B' is:\n" << B << endl << endl;
    // You should notice that the printed arrays include information about their
    // missing values and units.

    // All math works the same way that it does for the underlying Eigen objects.
    auto C = A + B;
    cout << "C = A + B:\n" << C << endl << endl;

    // Temporary objects are fully supported, though you may need to enclose quantities
    // in parentheses to help your C++ compiler understand what you mean to do.
    cout << "A - B:\n" << (A - B) << endl << endl;

    cout << "A * B:\n" << (A * B) << endl << endl;

    cout << "A / B:\n" << (A / B) << endl << endl;

    // In addition to performing math on arrays and arrays, you can do math on
    // arrays and scalars. Due to the C++ operator overload rules, however, the
    // array should go first.
    cout << "A + 2:\n" << (A + 2) << endl << endl;
    // cout << "2 + A:\n" << (2 + A) << endl << endl;  // This will not work.
    cout << "2A:\n" << (A * 2) << endl << endl;
    cout << "A/2:\n" << (A / 2) << endl << endl;
    cout << "B - 2:\n" << (B - 2) << endl << endl;
    cout << "B + (-3):\n" << (B + (-3)) << endl << endl;

    // More complex math is also supported:
    cout << "A^2 / (B+2):\n" << (A * A / (B + 2)) << endl << endl;

    // ------------------------------------------------
    // Units

    // Let's define a few more arrays. This time, we can attach units to the expressions.
    Eigen::Array<float, 1, 3> masses(3), distances(3), times(3);
    masses << 1, 1, 2;           // kg
    distances << 100, 200, 130;  // m
    times << 6.5, 12, 7;         // s
    auto M = ToEigenMath(std::move(masses), Units("kg"), -1000);
    auto D = ToEigenMath(std::move(distances), Units("m"), -1000);
    auto T = ToEigenMath(std::move(times), Units("s"), -1000);

    cout << "M\n" << M << endl << endl;
    cout << "D\n" << D << endl << endl;
    cout << "T\n" << T << endl << endl;

    // Let's calculate Velocity = distance / time
    cout << "Velocity:\n" << (D / T) << endl << endl;
    // Momentum
    cout << "Momentum:\n" << (M * D / T) << endl << endl;

    // For now, quantities of different units can be multiplied and divided, but they cannot
    // be added or subtracted. However, you can manually convert units to make them match.

    // To convert units, use the asUnits function.
    cout << "M (g)\n" << M.asUnits("g") << endl << endl;

    // Likewise, you cannot combine expressions that use fundamentally different
    // data types (float, double, int). To convert, use the cast<> function.
    cout << "D (as int)\n" << D.cast<int>() << endl << endl;

    // ------------------------------------------------
    // Missing values

    // Missing values are defined for every data type (int, float, double, char)
    // except for the boolean type.
    // Missing values are clingy: any math operation on a missing value
    // produces another missing value.

    ArrayXXf e(4, 3), f(4, 3);
    e << 1, 2, 3, 4, -99, 6, 7, 8, -99, 10, 11, 12;
    f << 1, 3, 5, 7, -99, 11, 13, 15, 17, 19, -99, 23;
    auto E = ToEigenMath(std::move(e), Units("m"), -99);
    auto F = ToEigenMath(std::move(f), Units("m"), -99);
    cout << "E:\n" << E << endl << endl;
    cout << "F:\n" << F << endl << endl;

    cout << "E+F:\n" << (E + F) << endl << endl;

    cout << "2*E:\n" << (E * 2) << endl << endl;

    // Missing values are correctly propagated across unit conversions and type casts.
    cout << "E (mm):\n" << E.asUnits("mm") << endl << endl;

    cout << "F (int):\n" << F.cast<int>() << endl << endl;

    // ------------------------------------------------
    // Comparison operators

    // In addition to the arithmetic operators +-*/, we implement the
    // comparison operators <, >, <=, >=, ==, !=, &&, and ||.

    // These operators return boolean matrices.

    // Note that comparisons involving missing values **always** return false.
    // *However*, comparison results are of type bool and do not propagate missing values.
    // This is an area of future work.

    cout << "(B <= A):\n" << (B <= A) << endl << endl;

    cout << "(E + F) >= 17:\n" << ((E + F) >= 17) << endl << endl;

    cout << "E<F:\n" << (E < F) << endl << endl;
    cout << "E + 1  > F:\n" << ((E + 1) > F) << endl << endl;

    cout << "E == E:\n" << (E == E) << endl << endl;
    cout << "A != B:\n" << (A != B) << endl << endl;

    cout << "(E == 6) || (E == 8):\n" << ((E == 6) || (E == 8)) << endl << endl;

    cout << "(E > 6) && (F > 18):\n" << ((E > 6) && (F > 18)) << endl << endl;

    // Missing values can be detected using the whereMissing() function.
    cout << "(E is missing):\n" << E.whereMissing() << endl << endl;

    // ------------------------------------------------
    // Selections / 'where' statements

    // Selections are powerful tools that implement the ternary operator:
    // (expression) ? value_if_true : value_if_false.

    // To select data, use the select(...) function.

    // To replace all values of A<6 with 0:
    cout << "((A<6).select(0,A)):\n" << ((A < 6).select(0, A)) << endl << endl;

    // To replace all A>6 with B:
    cout << "((A>6).select(B,A)):\n" << ((A > 6).select(B, A)) << endl << endl;

    // Selection statements do not even need to have the same variables
    // as the replacement expressions.
    cout << "((E.whereMissing()).select(A,A*2)):\n"
         << ((E.whereMissing()).select(A, A * 2)) << endl
         << endl;

    // Future:
    // To replace all values of A<6 with 0, and all other values with 3:
    // Note: this is the one odd select case that needs an explicit specification of
    // units and missing values. This is needed because no true/false matrices are involved,
    // and the select comparison is unitless and missing-value-less.
    //cout << "((A<6).select(0,3)):\n" << ((A<6).select(0,3, Units("1"), -99)) << endl << endl;

    // Future: To replace all odd numbers in A with the corresponding value in B:
    //cout << "((A%2).select(B,A):\n" << ((A%2).select(B,A)) << endl << endl;

    return 0;
  } catch (const std::exception &e) {
    cerr << "ERROR: " << e.what() << endl;
    return 1;
  }
}
