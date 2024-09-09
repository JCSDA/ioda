/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef CONTAINERS_IFRAMEDATA_H_
#define CONTAINERS_IFRAMEDATA_H_

#include <cstdint>
#include <string>

namespace osdf {
class IFrameData {
 public:
  IFrameData() {}

  IFrameData(IFrameData&&)                 = delete;
  IFrameData(const IFrameData&)            = delete;
  IFrameData& operator=(IFrameData&&)      = delete;
  IFrameData& operator=(const IFrameData&) = delete;

  virtual const std::string& getName(const std::int32_t) const = 0;
  virtual const std::int8_t getType(const std::int32_t) const = 0;
  virtual const std::int8_t getPermission(const std::int32_t) const = 0;
};
}  // namespace osdf

#endif  // CONTAINERS_IFRAMEDATA_H_
