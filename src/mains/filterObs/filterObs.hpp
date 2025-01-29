/*
 * (C) Copyright 2024 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef MAINS_FILTEROBS_H_
#define MAINS_FILTEROBS_H_

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include "eckit/config/LocalConfiguration.h"
#include "eckit/exception/Exceptions.h"

#include "oops/base/Observations.h"
#include "oops/base/ObsSpaces.h"
#include "oops/mpi/mpi.h"
#include "oops/runs/Application.h"
#include "oops/util/DateTime.h"
#include "oops/util/Duration.h"
#include "oops/util/Logger.h"
#include "oops/util/TimeWindow.h"

#include "ioda/core/IodaUtils.h"
#include "ioda/ObsSpace.h"

// This application can be run standalone and will perform simple
// filtering operations. The input is a set of ioda obs files, and
// the output is a single ioda obs file with the results of applying
// the filtering operations.
//
// Currently, the time window filtering is the only filtering operation.

// -----------------------------------------------------------------------------
/// \brief split full variable name into a group and name
/// \details This function will take a ioda variable name in the form of "group/variable"
/// and split it into the two pieces "group" and "variable". If fullVarName is set
/// to a string without a "/", then it is assumed that the groupName is "MetaData"
/// and varName is equal to fullVarName.
/// \param fullVarName full variable name in the format "group/variable"
/// \param groupName leading name before the "/" in the full name
/// \param varName leading name after the "/" in the full name
static void splitVarName(const std::string & fullVarName, std::string & groupName,
                         std::string & varName) {
  std::size_t slashPos = fullVarName.find("/");
  if (slashPos != std::string::npos) {
    groupName = fullVarName.substr(0, slashPos);
    varName = fullVarName.substr(slashPos + 1);
  } else {
    groupName = "MetaData";
    varName = fullVarName;
  }
}

// -----------------------------------------------------------------------------
  /// \brief generate receipt times
  /// \details This function will generate a contrived set of receipt times (for
  /// testing and demo purposes) by adding a delay, constrained by the delayMin
  /// and delayMax parameters, to each obs time stamp (MetaData/dateTime values).
  /// A modulo function will be applied to the dateTime values to determine a
  /// reproducible delay for each location, while making the delays added somewhat
  /// random-like.
  /// \param delayMin integer minimum delay value, in seconds
  /// \param delayMax integer maximum delay value, in seconds
  /// \param numDelayBins integer number of bins to split the delay range into
  /// \param receiptVarName name of new receipt time variable
static void generateReceiptTimes(const std::int64_t delayMin, const std::int64_t delayMax,
                          const int numDelayBins, const std::string & receiptVarName,
                          ioda::ObsSpace & obsdb) {
  // Check the parameters
  //     1. delayMin is less than delayMax
  //     2. numDelayBins is positive
  //     3. receiptVarName does not exist - if it does, issue a warning and
  //        skip the generation of receipt times
  bool paramsOkay = true;
  if (delayMin >= delayMax) {
    oops::Log::info() << "ERROR: generateReceiptTimes: YAML configuration 'delay min' must "
                      << "be less than 'delay max'" << std::endl;
    paramsOkay = false;
  }
  if (numDelayBins < 1) {
    oops::Log::info() << "ERROR: generateReceiptTimes: YAML configuration 'number of delay bins' "
                      << "must be a positive integer greater than zero" << std::endl;
    paramsOkay = false;
  }
  if (!paramsOkay) {
    throw eckit::BadParameter("Errors in YAML configuration", Here());
  }

  std::string grpName;
  std::string varName;
  splitVarName(receiptVarName, grpName, varName);
  if (obsdb.has(grpName, varName)) {
    oops::Log::info() << "WARNING: generateReceiptTimes: Receipt time variable already exists: "
                      << receiptVarName << std::endl
                      << "WARNING: Skipping the generate receipts times step." << std::endl;
  }

  // Get the time stamps for all of the locations, which are contained
  // in the MetaData/dateTime variable.
  if (!obsdb.has("MetaData", "dateTime")) {
    // MetaData/dateTime is expected to be in the input file
    std::string errMsg("input file does not contain the MetaData/dateTime variable");
    throw eckit::ReadError(errMsg, Here());
  }
  std::vector<util::DateTime> dateTimeVals;
  obsdb.get_db("MetaData", "dateTime", dateTimeVals);

  // Add the reproducible, but random-like, delays to the dateTimeVals
  // and write that out into the ObsSpace using the new variable name.
  util::DateTime refDateTime("1970-01-01T00:00:00Z");
  for (std::size_t i = 0; i < dateTimeVals.size(); ++i) {
    // Create a delay value that is inside the range delayMin to delayMax, then
    // add that value to the dateTime value, and simply write out the updated
    // dateTime values into the new receipt time variable.
    const std::int64_t modResult = (dateTimeVals[i] - refDateTime).toSeconds() % numDelayBins;
    const float modRangeFrac =
        static_cast<float>(modResult) / static_cast<float>(numDelayBins);
    const std::int64_t delay =
        delayMin + int64_t(modRangeFrac * static_cast<float>(delayMax - delayMin));
    dateTimeVals[i] += util::Duration(delay);
  }
  obsdb.put_db(grpName, varName, dateTimeVals);
}

// -----------------------------------------------------------------------------
  /// \brief Implementation of the receipt time filter.
  /// \details This filter is primarily (solely?) for dealing with contrived
  /// data that is being created for demo or research purposes. The idea is to
  /// set an "accept window" representing the arrival time window and to
  /// reject the obs that are outside that window. Note that the parameters of
  /// the filter are: an accept window, and a variable name containing the
  /// receipt times for each location.
  /// \param varName variable name holding the receipt times
  /// \param acceptWindow keep locations with receipt times inside this window
  /// \param obsdb ObsSpace object that the filter is being applied to
  static int applyReceiptTimeFilter(const std::string & receiptVarName,
                                    const util::TimeWindow & acceptWindow,
                                    ioda::ObsSpace & obsdb) {
    // Reject all locations that have a datetime stamp (MetaData/dateTime)
    // that is outside the accept window.
    int numRejected = 0;

    // The calling function has checked that both parameters (acceptWindow and
    // receiptTimeVariable) exist in the YAML configuration. Check to make sure
    // the receiptTimeVariable exists, and if so apply the filter.
    std::string grpName;
    std::string varName;
    std::vector<util::DateTime> receiptTimes;
    splitVarName(receiptVarName, grpName, varName);
    if (obsdb.has(grpName, varName)) {
      obsdb.get_db(grpName, varName, receiptTimes);
    } else {
      std::string errMsg = std::string("Receipt time variable does not exist: ") +
          receiptVarName;
      throw eckit::BadParameter(errMsg, Here());
    }

    // Walk through the receiptTimes comparing those to the acceptWindow.
    // Construct a boolean vector with 'true' values in the positions
    // where the receiptTimes entry is inside the acceptWindow. This
    // boolean vector can then be handed off to the ObsSpace::reduce
    // function to remove the rejected locations.
    std::vector<bool> keepTheseLocs = acceptWindow.createTimeMask(receiptTimes);
    numRejected = std::count(keepTheseLocs.begin(), keepTheseLocs.end(), false);

    // Only call reduce if there were any locations that were rejected.
    if (numRejected > 0) {
      obsdb.reduce(keepTheseLocs);
    }
    return numRejected;
  }

namespace ioda {

template <typename OBS> class FilterObs : public oops::Application {
  typedef oops::ObsSpace<OBS>         ObsSpace_;

 public:
  // ---------------------------------------------------------------------------
  explicit FilterObs(const eckit::mpi::Comm & comm = oops::mpi::world()) : Application(comm) {}
  // ---------------------------------------------------------------------------
  virtual ~FilterObs() {}
  // ---------------------------------------------------------------------------
  int execute(const eckit::Configuration & fullConfig) const override {
    //  Setup observation window
    const util::TimeWindow timeWindow(fullConfig.getSubConfiguration("time window"));
    oops::Log::info() << "Observation window: " << timeWindow << std::endl;

    // Grab config for the ObsSpace. Normally, obsdataout spec is
    // optional but in this case we want to make sure it is
    // included since we need to produce an output file.
    const eckit::LocalConfiguration obsconf(fullConfig, "obs space");
    if (!obsconf.has("obsdataout")) {
      std::string errMsg =
        std::string("ioda-filterObs: Must include 'obsdataout' spec ") +
        std::string("inside the 'obs space' spec");
      throw eckit::BadParameter(errMsg, Here());
    }

    // Create an ObsSpace object and the time window filtering will
    // happen automatically via the ioda reader.
    ObsSpace_ obsdb(obsconf, this->getComm(), timeWindow);

    // If specified, apply the receipt time filter - keep track of how many locations
    // were rejected due to the receipt time filter.
    int numReceiptTimeRejected = -1;
    const std::string receiptTimeFilterSpec("receipt time filter");
    if (fullConfig.has(receiptTimeFilterSpec)) {
      // Collect the configuration specs:
      //   variable name:
      //       name of the variable in the ObsSpace that holds the receipt times
      //   accept window:
      //       keep location if its receipt time is inside this window
      //   generate receipt times:
      //       optional spec that tells this filter to first generate (contrived) receipt times
      const eckit::LocalConfiguration filterConfig =
          fullConfig.getSubConfiguration(receiptTimeFilterSpec);
      const util::TimeWindow receiptAcceptWindow(
          filterConfig.getSubConfiguration("accept window"));
      const std::string receiptVarName = filterConfig.getString("variable name");

      // If there is a generate receipt times spec, do the generate action before
      // applying the filter. The generate receipt times section has two specs that
      // define a range of delays for constraining the generated times.
      //     delay min:
      //         minimum delay to add to the obs time stamp
      //     delay max:
      //         maximum delay to add to the obs time stamp
      //     module divisor:
      //         divisor to be used in the modulo method for generating times
      const std::string generateSpec("generate receipt times");
      if (filterConfig.has(generateSpec)) {
        const eckit::LocalConfiguration generateConfig =
            filterConfig.getSubConfiguration(generateSpec);
        std::int64_t delayMin = generateConfig.getInt64("delay min");
        std::int64_t delayMax = generateConfig.getInt64("delay max");
        int numDelayBins = generateConfig.getInt("number of delay bins");
        generateReceiptTimes(delayMin, delayMax, numDelayBins, receiptVarName, obsdb.obsspace());
      }

      numReceiptTimeRejected = applyReceiptTimeFilter(receiptVarName,
          receiptAcceptWindow, obsdb.obsspace());
    }

    // Display some stats
    oops::Log::info() << obsdb.obsname() << ": Total number of locations read: "
                      << obsdb.obsspace().sourceNumLocs() << std::endl;
    oops::Log::info() << obsdb.obsname() << ": Total number of locations kept: "
                      << obsdb.obsspace().globalNumLocs() << std::endl;
    oops::Log::info() << obsdb.obsname() << ": Number of locations outside time window: "
                      << obsdb.obsspace().globalNumLocsOutsideTimeWindow() << std::endl;
    if (numReceiptTimeRejected >= 0) {
      oops::Log::info() << obsdb.obsname()
                        << ": Number of locations rejected by the receipt time filter: "
                        << numReceiptTimeRejected << std::endl;
    }

    // Write the output file - already checked that we have an obsdataout spec
    obsdb.save();
    return 0;
  }

// -----------------------------------------------------------------------------
 private:
  std::string appname() const override {
    return "oops::FilterObs<" + OBS::name() + ">";
  }
};

}  // namespace ioda

#endif  // MAINS_FILTEROBS_H_
