/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*!
 * \defgroup ioda_cxx_ex_math Units and Math Examples
 * \brief How to perform efficient math that is array-based, respecting of units, and missing value-aware
 * \ingroup ioda_cxx_ex_adv
 * \details
 * 
 * IODA's units system is a light C++ wrapper around Unidata's udunits-2 C library.
 * 
 * @{
 * 
 * \file units.cpp
 * \details
 * 
 **/

#include "ioda/Units.h"

#include <exception>
#include <iostream>
#include <vector>

#include "ioda/Exception.h"

int main(int, char **) {
  using std::cerr;
  using std::cout;
  using std::endl;
  using namespace ioda;
  try {
    using namespace ioda::udunits;
    // Units are all defined at runtime and are read from a udunits xml file.
    // For most cases, we can just use the default system and its units.

    // To create an instance of a unit:
    Units kg("kg");
    Units m("m");
    Units s("s");
    Units N("N");
    Units Pa("Pa");

    // The grammar of the units string is discussed in the udunits manual:
    //   https://www.unidata.ucar.edu/software/udunits/udunits-2.2.28/udunits2lib.html#Parsing
    // Many units are built into the library. They are viewable here:
    //   https://www.unidata.ucar.edu/software/udunits/udunits-2.2.28/udunits2.html#Database

    // Compound units are also quite possible.
    Units N_v2("kg m / s^2");
    Units N_v3("kg.m / (s^2)");
    Units N_v4("kg*m/(s^2)");

    // The library can be a bit picky about spacing.
    // Ex: "kg m / (s^2)" and "kg*m/(s^2)" are valid but "kg * m / (s^2)" is not.
    // To check if your units were successfully parsed or not, use the isValid function.
    if (!kg.isValid()) throw Exception("Could not parse kg.", ioda_Here());
    if (!N_v2.isValid()) throw Exception("Could not parse Newtons v2.", ioda_Here());
    if (!N_v3.isValid()) throw Exception("Could not parse Newtons v3.", ioda_Here());
    if (!N_v4.isValid()) throw Exception("Could not parse Newtons v4.", ioda_Here());

    // Units may be easily printed.
    cout << "1 Newton is internally represented as: " << N << endl;
    cout << "N_v2 is " << N_v2 << endl;
    cout << "N_v3 is " << N_v3 << endl;
    cout << "N_v4 is " << N_v4 << endl;

    // You can perform basic math on units to get more complicated units.
    // C++ overloaded multiplicative operators * and / both work.
    // Parentheses work, too!
    auto m2       = m * m;
    auto m3       = m.raise(3);
    auto s2       = s * s;
    auto sqrt_s2  = s2.root(2);
    auto derivedN = kg * m / (s * s);

    if (!s2.isValid()) throw Exception("Failed to multiply units.", ioda_Here());

    cout << "m * m is " << m2 << endl;
    cout << "m * m * m is " << m3 << endl;
    cout << "s * s is " << s2 << endl;
    cout << "sqrt(s * s) is " << sqrt_s2 << endl;
    cout << "kg * m / (s * s) is " << derivedN << endl;

    // To check if two units are equal or nonequal, use the == and != operators.
    if (derivedN != N) throw Exception("1 N is 1 kg * m / s^2.", ioda_Here());
    if (m == s) throw Exception("1 meter is not equal to 1 second.", ioda_Here());
    if (N_v4 != N_v3) throw Exception("Units mismatch N_v4, N_v3.", ioda_Here());
    if (N_v4 != N_v2) throw Exception("Units mismatch N_v4, N_v2.", ioda_Here());
    if (N_v4 != N) throw Exception("Units mismatch N_v4, N.", ioda_Here());
    if (N_v4 != derivedN) throw Exception("Units mismatch N_v4, derivedN.", ioda_Here());

    // Units with prefixes work.
    auto cm = Units("cm");

    // To check if units are convertible, use the isConvertibleWith function.
    if (!cm.isConvertibleWith(m))
      throw Exception("cm should be convertible with m.", ioda_Here());
    else
      cout << cm << " is convertible with " << m << endl;

    if (Units("inches").isConvertibleWith(Units("millimeters")))
      cout << "inches are convertible to millimeters.\n";

    // To convert values with compatible units, you can ask for a converter.
    auto in2mm = Units("inches").getConverterTo(Units("mm"));

    // The converter provides methods to convert floats, doubles,
    // arrays of floats, and arrays of doubles.
    std::vector<float> lengths_inches{1, 2, 3.5, 5};
    std::vector<float> lengths_mm(lengths_inches.size());
    in2mm->convert(lengths_inches.data(), lengths_inches.size(), lengths_mm.data());

    cout << "Converting inches to mm:\n";
    for (size_t i = 0; i < lengths_inches.size(); ++i)
      cout << "\t" << lengths_inches[i]
           << ((lengths_inches[i] == 1.0f) ? " inch equals " : " inches equal ") << lengths_mm[i]
           << " millimeters.\n";

    return 0;
  } catch (const std::exception &e) {
    cerr << "ERROR: " << e.what() << endl;
    return 1;
  }
}
