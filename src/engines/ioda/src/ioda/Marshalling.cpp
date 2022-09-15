/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include <chrono>
#include <ctime>    // tzset
#include <iomanip>  // std::put_time
#include <iostream>
#include <sstream>

#include "ioda/Types/Marshalling.h"
#include "ioda/Attributes/Has_Attributes.h"

namespace {

/// \brief Moderator for timezone (TZ) environment variable manipulation.
/// \details This is needed because setenv is required for the ancient C and C++
///   time functions that convert strings to std::tm objects. Any use of setenv should
///   be considered totally buggy, but it is unavoidable in cases where we perform I/O.
///
///   This class will change the timezone and then will change it back upon leaving
///   scope. This is still totally thread-unsafe, unfortunately.
/// \see std::get_time references for the many issues with the old functions.
/// \todo Remove all of this as soon as we have C++20 support.
class TZ_lock {
  std::string old_tz_;
public:
  TZ_lock(const char* TZ)
  {
    const char* old_tz = getenv("TZ");
    if (old_tz)
      old_tz_.assign(old_tz);
    setenv("TZ", TZ, 1);
    tzset();
  }
  ~TZ_lock() {
    if (old_tz_.size()) {
      setenv("TZ", old_tz_.data(), 1);
      tzset();
    }
  }
};

}  // end anonymous namespace

namespace ioda {

namespace detail {

/// \todo Refactor once C++20 is available.
ioda::Types::Chrono_Time_Point_t getEpoch(const Has_Attributes *atts)
{
  if (atts) {
    if (atts->exists("units")) {
      std::string units = atts->open("units").read<std::string>();
      // Units should be in format of "seconds since *****".
      // Remove the "seconds since " part and convert the remainder.
      const std::string prefix = "seconds since ";
      if (units.substr(0, prefix.size()) == prefix) {
        TZ_lock TZ{"UTC"};
        std::string sEpoch = units.substr(prefix.size());

        std::istringstream isEpoch(sEpoch);
        struct std::tm tm{};
        isEpoch >> std::get_time(&tm, Types::Chrono_Time_Format);
        if (isEpoch.fail()) throw Exception("Parse failed.", ioda_Here());

        // Always needs to be set, per mktime's interaction with get_time.
        // See https://en.cppreference.com/w/cpp/chrono/c/mktime
        tm.tm_isdst = 0;

        std::time_t time = mktime(&tm);
        ioda::Types::Chrono_Time_Point_t epoch =
            std::chrono::time_point_cast<ioda::Types::Chrono_Duration_t>(
            std::chrono::system_clock::from_time_t(time));
        return epoch;
      }
    }
  }
  return ioda::Types::Chrono_Time_Point_t{};
};

}  // end namespace detail
}  // end namespace ioda
