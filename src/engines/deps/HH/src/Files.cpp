/*
 * (C) Copyright 2017-2020 Ryan Honeyager (ryan@honeyager.info)
 *
 * This software is licensed under the terms of the BSD 2-Clause License.
 * For details, see the LICENSE file.
 */
#include "HH/Files.hpp"
#include <mutex>

namespace HH {
	using namespace HH::Handles;
	using namespace HH::Types;

	File::File() : File(HH_hid_t{}) {}
	File::File(HH_hid_t hnd) : Group(hnd), base(hnd), atts(hnd), dsets(hnd) {}
	File::~File() = default;
	HH_hid_t File::get() const { return base; }

	H5F_info_t& File::get_info(H5F_info_t& info) const {
		herr_t err = H5Fget_info(base(), &info);
		HH_Expects(err >= 0);
		return info;
	}
	HH_NODISCARD File File::openFile(
		const std::string& filename,
		unsigned int FileOpenFlags,
		HH_hid_t FileAccessPlist)
	{
        hid_t isFile = H5Fis_hdf5(filename.c_str());
        HH_Expects(isFile > 0);
		hid_t res = H5Fopen(filename.c_str(), FileOpenFlags, FileAccessPlist());
		HH_Expects(res >= 0);
		return File(HH_hid_t(res, Closers::CloseHDF5File::CloseP));
	}
	HH_NODISCARD File File::createFile(
		const std::string& filename,
		unsigned int FileCreateFlags,
		HH_hid_t FileCreationPlist,
		HH_hid_t FileAccessPlist)
	{
		hid_t res = H5Fcreate(filename.c_str(), FileCreateFlags,
			FileCreationPlist(), FileAccessPlist());
		HH_Expects(res >= 0);
		return File(HH_hid_t(res, Closers::CloseHDF5File::CloseP));
	}
	/// \todo Make truly random. Generate UUIDs.
	HH_NODISCARD std::string File::genUniqueFilename()
	{
		static int i = 0;
		static std::mutex m;

		std::lock_guard<std::mutex> l{ m };
		std::ostringstream out;
		out << "HH-temp-" << i << ".h5"; // Change this if debugging to file.
		++i;
		return out.str();
	}

	HH_NODISCARD File File::create_file_mem(
		const std::string& filename,
		size_t increment_len,
		bool flush_on_close)
	{
		using namespace HH;
		hid_t plid = H5Pcreate(H5P_FILE_ACCESS);
		Expects(plid >= 0);
		HH_hid_t pl(plid, Handles::Closers::CloseHDF5PropertyList::CloseP);

		const auto h5Result = H5Pset_fapl_core(pl.get(), increment_len, flush_on_close);
		HH_Expects(h5Result >= 0 && "H5Pset_fapl_core failed");

		// This new memory-only dataset needs to always be writable. The flags parameter
		// has little meaning in this context.
		/// \todo Check if truncation actually removes the file on the disk!!!!!
		hid_t res = H5Fcreate(filename.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, pl());
		HH_Expects(res >= 0);
		return File(HH_hid_t(res, Closers::CloseHDF5File::CloseP));
	}
} // namespace HH

