/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "ioda/Engines/HH.h"

#include <mutex>
#include <random>
#include <sstream>

#include "ioda/Engines/HH/HH-attributes.h"
#include "ioda/Engines/HH/HH-groups.h"
#include "ioda/Group.h"
#include "ioda/defs.h"

namespace ioda {
namespace Engines {
namespace HH {
// Adapted from https://lowrey.me/guid-generation-in-c-11/
unsigned int random_char() {
  static std::random_device rd;
  static std::mt19937 gen(rd());
  static std::uniform_int_distribution<> dis(0, 255);
  return dis(gen);
}

std::string generate_hex(const unsigned int len) {
  std::stringstream ss;
  for (unsigned i = 0; i < len; i++) {
    const auto rc = random_char();
    std::stringstream hexstream;
    hexstream << std::hex << rc;
    auto hex = hexstream.str();
    ss << (hex.length() < 2 ? '0' + hex : hex);
  }
  return ss.str();
}

std::string genUniqueName() {
  // GUIDs look like: {CD1A91C6-9C1B-454E-AD1C-977F4C72A01C}.
  // We use these for the file name because they are quite unique.
  // HDF5 needs unique names, otherwise it might open the same memory file twice.

  static std::mutex m;

  std::lock_guard<std::mutex> l{m};

  std::array<std::string, 5> uuid = {generate_hex(8), generate_hex(4), generate_hex(4), generate_hex(4),
                                     generate_hex(12)};
  std::string res = uuid[0] + "-" + uuid[1] + "-" + uuid[2] + "-" + uuid[3] + "-" + uuid[4] + ".hdf5";
  return res;
}

Group createMemoryFile(const std::string& filename, BackendCreateModes mode, bool flush_on_close,
                       size_t increment_len) {
  static const std::map<BackendCreateModes, unsigned int> m{
    {BackendCreateModes::Truncate_If_Exists, H5F_ACC_TRUNC},
    {BackendCreateModes::Fail_If_Exists, H5F_ACC_CREAT}};

  hid_t plid = H5Pcreate(H5P_FILE_ACCESS);
  Expects(plid >= 0);
  ::HH::HH_hid_t pl(plid, ::HH::Handles::Closers::CloseHDF5PropertyList::CloseP);

  //if (0 > H5Pset_fapl_core(pl.get(), increment_len, flush_on_close))
  //  throw;  // jedi_throw.add("Reason", "H5Pset_fapl_core failed");
  // H5F_LIBVER_V18, H5F_LIBVER_V110, H5F_LIBVER_V112, H5F_LIBVER_LATEST.
  // Note: this propagates to any files flushed to disk.
  //if (0 > H5Pset_libver_bounds(pl.get(), H5F_LIBVER_V18, H5F_LIBVER_LATEST))
  //  throw;  // jedi_throw.add("Reason", "H5Pset_libver_bounds failed");

  ::HH::File f = ::HH::File::createFile(filename, m.at(mode), H5P_DEFAULT, pl);

  auto backend =
    std::make_shared<detail::Engines::HH::HH_Group_Backend>(f, getCapabilitiesInMemoryEngine(), f);
  return ::ioda::Group{backend};
}

Group createFile(const std::string& filename, BackendCreateModes mode) {
  static const std::map<BackendCreateModes, unsigned int> m{
    {BackendCreateModes::Truncate_If_Exists, H5F_ACC_TRUNC},
    {BackendCreateModes::Fail_If_Exists, H5F_ACC_CREAT}};

  hid_t plid = H5Pcreate(H5P_FILE_ACCESS);
  Expects(plid >= 0);
  ::HH::HH_hid_t pl(plid, ::HH::Handles::Closers::CloseHDF5PropertyList::CloseP);
  // H5F_LIBVER_V18, H5F_LIBVER_V110, H5F_LIBVER_V112, H5F_LIBVER_LATEST.
  // Note: this propagates to any files flushed to disk.
  //if (0 > H5Pset_libver_bounds(pl.get(), H5F_LIBVER_V18, H5F_LIBVER_LATEST))
  //  throw;  // jedi_throw.add("Reason", "H5Pset_libver_bounds failed");

  ::HH::File f = ::HH::File::createFile(filename, m.at(mode), H5P_DEFAULT, pl);

  auto backend = std::make_shared<detail::Engines::HH::HH_Group_Backend>(f, getCapabilitiesFileEngine(), f);
  return ::ioda::Group{backend};
}

Group openFile(const std::string& filename, BackendOpenModes mode) {
  static const std::map<BackendOpenModes, unsigned int> m{{BackendOpenModes::Read_Only, H5F_ACC_RDONLY},
                                                          {BackendOpenModes::Read_Write, H5F_ACC_RDWR}};
  ::HH::File f = ::HH::File::openFile(filename, m.at(mode));

  auto backend = std::make_shared<detail::Engines::HH::HH_Group_Backend>(f, getCapabilitiesFileEngine(), f);

  return ::ioda::Group{backend};
}

Group openMemoryFile(const std::string& filename, BackendOpenModes mode, bool flush_on_close,
                     size_t increment_len) {
  static const std::map<BackendOpenModes, unsigned int> m{{BackendOpenModes::Read_Only, H5F_ACC_RDONLY},
                                                          {BackendOpenModes::Read_Write, H5F_ACC_RDWR}};

  hid_t plid = H5Pcreate(H5P_FILE_ACCESS);
  Expects(plid >= 0);
  ::HH::HH_hid_t pl(plid, ::HH::Handles::Closers::CloseHDF5PropertyList::CloseP);

  const auto h5Result = H5Pset_fapl_core(pl.get(), increment_len, flush_on_close);
  if (h5Result < 0) throw;  // jedi_throw.add("Reason", "H5Pset_fapl_core failed");

  ::HH::File f = ::HH::File::openFile(filename, m.at(mode), pl);

  auto backend =
    std::make_shared<detail::Engines::HH::HH_Group_Backend>(f, getCapabilitiesInMemoryEngine(), f);

  return ::ioda::Group{backend};
}

Capabilities getCapabilitiesFileEngine() {
  static Capabilities caps;
  static bool inited = false;
  if (!inited) {
    caps.canChunk = Capability_Mask::Supported;
    caps.canCompressWithGZIP = Capability_Mask::Supported;
    caps.MPIaware = Capability_Mask::Supported;

    bool canSZIP = ::HH::CanUseSZIP<int>();
    caps.canCompressWithSZIP = (canSZIP) ? Capability_Mask::Supported : Capability_Mask::Unsupported;
  }

  return caps;
}

Capabilities getCapabilitiesInMemoryEngine() {
  static Capabilities caps;
  static bool inited = false;
  if (!inited) {
    caps.canChunk = Capability_Mask::Supported;
    caps.canCompressWithGZIP = Capability_Mask::Supported;
    caps.MPIaware = Capability_Mask::Unsupported;

    bool canSZIP = ::HH::CanUseSZIP<int>();
    caps.canCompressWithSZIP = (canSZIP) ? Capability_Mask::Supported : Capability_Mask::Unsupported;
  }

  return caps;
}

}  // namespace HH
}  // namespace Engines
}  // namespace ioda
