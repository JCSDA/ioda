/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*!
 * \ingroup ioda_cxx_ex_types
 *
 * @{
 *
 * \defgroup ioda_cxx_ex_types_chrono std::chrono objects
 * \brief Reading and writing time_point and duration objects.
 * 
 * @{
 * 
 * \file chrono.cpp
 * \details
 * 
 * This example shows how IODA can read and write time points and durations. It is
 * built off of the array_from_struct example, so please reference that example to
 * understand what is going on.
 *
 * Most of the important logic is already built into IODA, since std::chrono is in the
 * STL. The Python interface also works with Python datetime objects, with the caveat
 * that time zone information is lost upon conversion. IODA always assumes that
 * you are in UTC time.
 **/

#include <ctime>
#include <chrono>
#include <iomanip>  // std::put_time
#include <iostream>
#include <vector>

#include "ioda/Engines/Factory.h"
#include "ioda/Exception.h"
#include "ioda/Group.h"

#if __cplusplus < 202002L
/// \brief Stringify a time point. Used only as an example.
/// \note This code will be superseded with the switch to C++20, as
///   C++20 adds ostream operators for all time types.
template <class CharT, class Traits, class Clock_T>
std::basic_ostream<CharT, Traits>&
operator<<(std::basic_ostream<CharT, Traits>& os,
           const std::chrono::time_point<Clock_T>& tp)
{
  const std::time_t t = Clock_T::to_time_t(tp);
  os << std::put_time(std::gmtime(&t), ioda::Types::Chrono_Time_Format);
  return os;
}
#endif

/// \brief The main program. Reads and writes data.
int main(int argc, char** argv) {
  using namespace ioda;
  using namespace std;
  try {
    // Use the HDF5 file backend by default.
    auto f = Engines::constructFromCmdLine(argc, argv, "chrono.hdf5");

    // Here is the datum that is read and written.
    // We truncate to the nearest second because we presently lack support for sub-second intervals.
    // This may change when C++20 is adopted.
    const std::vector<std::chrono::time_point<std::chrono::system_clock>> times {
      std::chrono::time_point_cast<std::chrono::seconds>(std::chrono::system_clock::now())
    };

    // Write the data.
    // To write time data, we encode as a duration from a reference.
    Variable var = f.vars.create<std::chrono::time_point<std::chrono::system_clock>>("now");
    var.setIsDimensionScale("time");

    // Write the units.
    // For netCDF compatability, the units:
    // - must be a fixed-length string,
    // - must have a padded null-termination byte at the end, and
    // - must be written using a simple (not scalar) dataspace.
    // Units may be written in either UTF-8 or ASCII. udunits supports both.
    // Note: VariableCreationParameters::defaulted<Chrono_Time_Point_default_t>() could
    //   be specialized to set default units for the epoch, but let's be generic for now.
    // TODO(ryan): Once the future ioda Type System refactor PR is merged, then the syntax
    // for creating a fixed-length string attribute will be slightly simplified.
    std::string epochRef{"seconds since 1970-01-01T00:00:00Z"};
    Type tEpochRef = var.atts.getTypeProvider()->makeStringType(
		         typeid(std::string),  // unnecessary after the type system upgrade,
			 epochRef.size()+1);   // the size of the null-terminated string
    var.atts.create("units",    // attribute name
		    tEpochRef,  // use the custom fixed-length string type
		    {})         // use a simple dataspace, not a scalar dataspace
            .write<std::string>(epochRef);

    var.write<std::chrono::time_point<std::chrono::system_clock>>(times);

    // Read and check the variable
    std::vector<std::chrono::time_point<std::chrono::system_clock>> read_times;
    var.read<std::chrono::time_point<std::chrono::system_clock>>(read_times);


    if (read_times.size() != times.size()) throw Exception("Read size mismatch.", ioda_Here());
    for (size_t i=0; i < times.size(); ++i) {
      cout << "Written: " << times[i] << "    Read: " << read_times[i] << endl;
      if (read_times[i] != times[i]) throw Exception("Read mismatch", ioda_Here()).add("index", i);
    }

    return 0;
  } catch (const std::exception& e) {
    ioda::unwind_exception_stack(e);
    return 1;
  }
}
