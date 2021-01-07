#pragma once
/*
 * (C) Copyright 2017-2020 Ryan Honeyager (ryan@honeyager.info)
 *
 * This software is licensed under the terms of the BSD 2-Clause License.
 * For details, see the LICENSE file.
 */
#include <hdf5.h>
#include <hdf5_hl.h>

#include <cstdio>
#include <gsl/gsl-lite.hpp>
#include <string>
#include <tuple>

#include "./defs.hpp"
#include "Attributes.hpp"
#include "Datasets.hpp"
#include "Errors.hpp"
#include "Groups.hpp"
#include "Handles.hpp"
#include "Types.hpp"

namespace HH {
using namespace HH::Handles;
using namespace HH::Types;
using std::initializer_list;
using std::tuple;

struct HH_DL File : public Group {
private:
  HH_hid_t base;

public:
  File();
  explicit File(HH_hid_t hnd);
  virtual ~File();
  HH_hid_t get() const;

  Has_Attributes atts;
  // Has_Groups grps;
  Has_Datasets dsets;

  H5F_info_t& get_info(H5F_info_t& info) const;

  /// \note Should ideally be open, but the Group::open functions get masked.
  HH_NODISCARD static File openFile(const std::string& filename, unsigned int FileOpenFlags,
                                    HH_hid_t FileAccessPlist = H5P_DEFAULT);

  /// \note Should ideally be create, but the Group::create functions get masked.
  HH_NODISCARD static File createFile(const std::string& filename, unsigned int FileCreateFlags,
                                      HH_hid_t FileCreationPlist = H5P_DEFAULT,
                                      HH_hid_t FileAccessPlist = H5P_DEFAULT);

  /// \brief Creates a new file image (i.e. a file that exists purely in memory)
  HH_NODISCARD static File create_file_mem(const std::string& filename,
                                           size_t increment_len = 1000000,  // 1 MB
                                           bool flush_on_close = false);

  HH_NODISCARD static std::string genUniqueFilename();

};
}  // namespace HH
