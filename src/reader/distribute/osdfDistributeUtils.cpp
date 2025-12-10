/*
 * (C) Copyright 2025 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/// \file osdfDistributeUtils.cpp
/// \brief Utilities for distributing data in IFrame (OSDF) containers across MPI tasks

#include "osdfDistributeUtils.hpp"

#include <unordered_map>
#include <numeric>
#include <utility>

#include "eckit/exception/Exceptions.h"
#include "ioda/containers/FrameUtils.h"

namespace ioda {
namespace reader {

namespace detail {
  using std::to_string;

  /// An overload of to_string() taking a string and returning the same string.
  std::string to_string(std::string s) {
    return s;
  }
}  // namespace detail

void osdfBuildObsGroupingKeys(const osdf::IFrame & srcFrame,
                              const std::vector<std::string> & obsGroupVarList,
                              std::vector<std::string> & groupingKeys) {
    // Walk though each variable and construct the segments of the key values (strings)
    // Append the segments as each variable is encountered.
    for (std::size_t i = 0; i < obsGroupVarList.size(); ++i) {
        // Retrieve the variable values from the obs source and convert
        // those values to strings. Then append those "value" strings from each
        // variable to form the grouping keys.
        std::string obsGroupVarName = obsGroupVarList[i];
        std::string varName = std::string("MetaData/") + obsGroupVarName;
        ASSERT_MSG(srcFrame.hasColumn(varName),
                "Obs grouping variable " + varName + " not found in source frame.");
        auto columnType = srcFrame.getColumnType(varName);

        osdf::FrameUtils::callWithSupportedType(
                columnType,
                [&](auto typeDiscriminator) {
                    typedef decltype(typeDiscriminator) T;
                    std::vector<T> varValues;
                    srcFrame.getColumn(varName, varValues);
                    for (std::size_t j = 0; j < srcFrame.numRows(); ++j) {
                        std::string keySegment = detail::to_string(varValues[j]);
                        if (i == 0) {
                            groupingKeys[j] = keySegment;
                        } else {
                            groupingKeys[j] += ":" + keySegment;
                        }
                    }
                });
    }
}

void osdfAssignRecordNumbers(const osdf::IFrame & srcFrame,
                             const std::vector<std::string> & obsGroupVarList,
                             std::vector<std::size_t> & sourceRecNums) {
    // If the obsGroupVarList is empty, then the obs grouping feature is not being
    // used and the record number assignment can simply be sequential numbering
    // starting with zero. Otherwise, assign unique record numbers to each unique
    // combination of the values in the obsGroupVarList.
    const std::size_t locSize = srcFrame.numRows();
    sourceRecNums.resize(locSize);

    if (obsGroupVarList.size() == 0) {
        // Do not apply obs grouping. Simply assign sequential numbering.
        std::iota(sourceRecNums.begin(), sourceRecNums.end(), 0);
    } else {
        // Apply obs grouping. First convert all of the group variable data values for this
        // frame into string key values. This is done in one call to minimize accessing the
        // frame data for the grouping variables.
        std::vector<std::string> obsGroupingKeys(locSize);
        osdfBuildObsGroupingKeys(srcFrame, obsGroupVarList, obsGroupingKeys);

        std::size_t recnum = 0;
        std::unordered_map<std::string, std::size_t> obsGroupingMap;
        for (std::size_t i = 0; i < locSize; ++i) {
            if (obsGroupingMap.find(obsGroupingKeys[i]) == obsGroupingMap.end()) {
                // key is not present in the map -> assign current record number to
                // the current key and move to the next record number
                obsGroupingMap.insert(
                    std::pair<std::string, std::size_t>(obsGroupingKeys[i], recnum));
                recnum++;
            }
            sourceRecNums[i] = obsGroupingMap.at(obsGroupingKeys[i]);
        }
    }
}

void osdfSelectRankData(const std::unique_ptr<osdf::IFrame> &  srcGlobalFrame,
                        std::unique_ptr<osdf::IFrame> &  destRankFrame,
                        const std::string & columnName,
                        const std::vector<std::size_t> & sourceLocIndices) {
    ASSERT(srcGlobalFrame->hasColumn(columnName));
    ASSERT(!destRankFrame->hasColumn(columnName));  // This function is intended to add a new column
    auto columnType = srcGlobalFrame->getColumnType(columnName);
    osdf::FrameUtils::callWithSupportedType(
        columnType,
        [&](auto typeDiscriminator) {
            using T = decltype(typeDiscriminator);
            std::vector<T> srcValues;
            srcGlobalFrame->getColumn(columnName, srcValues);
            std::vector<T> destValues;
            destValues.reserve(sourceLocIndices.size());
            for (auto globalIndex : sourceLocIndices) {
                destValues.push_back(srcValues[globalIndex]);
            }
            destRankFrame->appendNewColumn(columnName, destValues);
        });
}

}  // namespace reader
}  // namespace ioda
