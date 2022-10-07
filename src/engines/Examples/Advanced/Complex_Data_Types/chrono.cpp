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

#include "ioda/Engines/EngineUtils.h"
#include "ioda/Exception.h"
#include "ioda/Group.h"

#if __cplusplus < 202002L
/// \brief Stringify a time point. Used only as an example.
/// \note This code will be superseded with the switch to C++20, as
///   C++20 adds ostream operators for all time types.
template <class Clock_T, class Duration_T>
std::ostream & operator<<(std::ostream & os,
                          const std::chrono::time_point<Clock_T, Duration_T> & tp) {
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

    // Force UTC time zone
    setenv("TZ", "UTC", 1);

    // Create the epoch: Jan 1, 1970, 0Z
    // Examples of initializing a std::chrono::time_point typically involve 3 steps:
    //    1. Create a std::tm with the desired date, time
    //    2. Convert the std::tm object to a std::time_t object
    //    3. Use the std::chrono::system_clock::from_time_t function to convert to
    //       to a std::chrono::time_point.
    //
    // Notes:
    //   In this case we need to cast the from_time_t result to our ioda time_point spec
    //   so the std::chrono::time_point_cast is used to do that.
    //   The std::tm (https://en.cppreference.com/w/cpp/chrono/c/tm) is a bit non-intuitive
    //     The year is expressed as an offset from the year 1900 (thus the "- 1900" in the
    //     code below).
    //     The day of the month is numbered from 1 to 31, while the month, hour, min, sec
    //     are numbered starting with zero.
    //     The tm_isdst set to zero means to use standard time instead of daylight
    //     savings time (dst).
    ioda::Types::Chrono_Time_Point_t epoch;
    struct std::tm timeTemp{};
    timeTemp.tm_year = 1970 - 1900;
    timeTemp.tm_mon = 0;
    timeTemp.tm_mday = 1;
    timeTemp.tm_hour = 0;
    timeTemp.tm_min = 0;
    timeTemp.tm_sec = 0;
    timeTemp.tm_isdst = 0;
    std::time_t tt = std::mktime(&timeTemp);
    epoch = std::chrono::time_point_cast<ioda::Types::Chrono_Duration_t>
            (std::chrono::system_clock::from_time_t(tt));

    // Create a fill value: Jan 1, 2200, 0Z
    // This datetime is used since that is what the ioda python API uses.
    // Borrow the settings in timeTemp from above.
    ioda::Types::Chrono_Time_Point_t absFillVal;
    timeTemp.tm_year = 2200 - 1900;
    tt = std::mktime(&timeTemp);
    absFillVal = std::chrono::time_point_cast<ioda::Types::Chrono_Duration_t>
                 (std::chrono::system_clock::from_time_t(tt));
    ioda::Types::Chrono_Duration_t fillVal = absFillVal - epoch;

    // This may change when C++20 is adopted.
    const std::vector<ioda::Types::Chrono_Time_Point_t> times {
      std::chrono::time_point_cast<ioda::Types::Chrono_Duration_t>
          (std::chrono::system_clock::now()),
      absFillVal
    };

    // Write the data.
    // To write time data, we encode as a duration from a reference.
    VariableCreationParameters params;
    params.setFillValue<ioda::Types::Chrono_Duration_t>(fillVal);
    Variable var = f.vars.create<ioda::Types::Chrono_Time_Point_t>("now", {2}, {2}, params);
    var.setIsDimensionScale("time");

    // Write the units.
    // For netCDF compatability, the units:
    // - must be a fixed-length string,
    // - must have a padded null-termination byte at the end, and
    // - must be written using a simple (not scalar) dataspace.
    // Units may be written in either UTF-8 or ASCII. udunits supports both.
    // Note: VariableCreationParameters::defaulted<Chrono_Time_Point_t>() could
    //   be specialized to set default units for the epoch, but let's be generic for now.
    // TODO(ryan): Once the future ioda Type System refactor PR is merged, then the syntax
    // for creating a fixed-length string attribute will be slightly simplified.
    //
    // Get the epoch value from the epoch chrono object
    tt = std::chrono::system_clock::to_time_t(epoch);
    struct std::tm * epochTime;
    epochTime = std::gmtime(&tt);
    char epochString[21];
    std::strftime(epochString, 21, ioda::Types::Chrono_Time_Format, epochTime);
    std::string epochRef = std::string("seconds since ") + std::string(epochString);
    Type tEpochRef = var.atts.getTypeProvider()->makeStringType(
		         typeid(std::string),  // unnecessary after the type system upgrade,
			 epochRef.size()+1);   // the size of the null-terminated string
    var.atts.create("units",    // attribute name
		    tEpochRef,  // use the custom fixed-length string type
		    {})         // use a simple dataspace, not a scalar dataspace
            .write<std::string>(epochRef);

    var.write<ioda::Types::Chrono_Time_Point_t>(times);

    // Read and check the variable
    std::vector<ioda::Types::Chrono_Time_Point_t> read_times;
    var.read<ioda::Types::Chrono_Time_Point_t>(read_times);


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
