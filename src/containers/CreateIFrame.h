/*
 * (C) Crown copyright 2026, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#pragma once

#include <memory>
#include <string>

#include "FrameCols.h"
#include "FrameRows.h"
#include "IFrame.h"

namespace osdf {

inline std::unique_ptr<osdf::IFrame> createIFrame(std::string frameType) {
  if (frameType == "FrameCols") {
    return std::make_unique<osdf::FrameCols>();
  } else if (frameType == "FrameRows") {
    return std::make_unique<osdf::FrameRows>();
  } else {
    throw eckit::BadParameter("Unknown data frame type: " + frameType, Here());
  }
}

}  // namespace osdf

