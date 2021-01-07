#pragma once
/*
 * (C) Copyright 2017-2020 Ryan Honeyager (ryan@honeyager.info)
 *
 * This software is licensed under the terms of the BSD 2-Clause License.
 * For details, see the LICENSE file.
 */

#include <hdf5.h>

#include <gsl/gsl-lite.hpp>
#include <string>
#include <vector>

#include "./defs.hpp"
#include "Attributes.hpp"
#include "Datasets.hpp"
#include "Errors.hpp"
#include "Funcs.hpp"
#include "Handles.hpp"
#include "Types.hpp"

namespace HH {
using namespace HH::Handles;
using namespace HH::Types;
using std::initializer_list;
using std::tuple;

struct HH_DL GroupParameterPack {
  AttributeParameterPack atts;
  HH_hid_t GroupAccessPlist = H5P_DEFAULT;

  struct HH_DL GroupCreationPListProperties {
    HH_hid_t GroupCreationPlistCustom = H5P_DEFAULT;
    bool UseCustomGroupCreationPlist = false;
    bool set_link_creation_order = true;

    HH_hid_t generateGroupCreationPlist() const;
  } groupCreationProperties;

  struct HH_DL LinkCreationPListProperties {
    HH_hid_t LinkCreationPlistCustom = H5P_DEFAULT;
    bool UseCustomLinkCreationPlist = false;

    bool CreateIntermediateGroups = false;
    HH_hid_t generateLinkCreationPlist() const;
  } linkCreationProperties;

  GroupParameterPack();
  GroupParameterPack(const AttributeParameterPack& a, HH_hid_t GroupAccessPlist = H5P_DEFAULT);
};

struct HH_DL Group {
private:
  HH_hid_t base;

public:
  Group();
  explicit Group(HH_hid_t hnd);
  virtual ~Group();
  HH_hid_t get() const;

  H5G_info_t& get_info(H5G_info_t& info) const;

  Has_Attributes atts;
  Has_Datasets dsets;

  static bool isGroup(HH_hid_t obj);
  bool isGroup() const;

  /// \brief List all groups under this group
  std::vector<std::string> list() const;

  /// \brief Does a group exist at the specified path?
  bool exists(const std::string& name, HH_hid_t LinkAccessPlist = H5P_DEFAULT) const;

  /// \brief Create a group
  /// \returns an invalid handle on failure.
  /// \returns a scoped handle to the group on success
  Group create(const std::string& name, GroupParameterPack gp = GroupParameterPack());

  /// \brief Open a group
  /// \returns an invalid handle if an error occurred
  /// \returns a scoped handle to the group upon success
  /// \note It is possible to have multiple handles opened for the group
  /// simultaneously. HDF5 has its own reference counting implementation.
  Group open(const std::string& name, HH_hid_t GroupAccessPlist = H5P_DEFAULT);

  /// \brief Mount a file into a group
  /// \returns >=0 on success, negative on failure
  void mount(const std::string& destination_groupname, HH_hid_t source_file,
             HH_hid_t FileMountPlist = H5P_DEFAULT);

  /// \brief Unmount a file from a group
  /// \returns >=0 success, negative on failure
  void unmount(const std::string& mountpoint);
};

typedef Group Has_Groups;
}  // namespace HH
