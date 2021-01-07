/*
 * (C) Copyright 2017-2020 Ryan Honeyager (ryan@honeyager.info)
 *
 * This software is licensed under the terms of the BSD 2-Clause License.
 * For details, see the LICENSE file.
 */
#include "HH/Groups.hpp"

namespace HH {
	using namespace HH::Handles;
	using namespace HH::Types;

	HH_hid_t GroupParameterPack::GroupCreationPListProperties::generateGroupCreationPlist() const {
		if (UseCustomGroupCreationPlist) return GroupCreationPlistCustom;
		hid_t plid = H5Pcreate(H5P_GROUP_CREATE);
		if (plid < 0) throw;  // HH_throw;
		if (set_link_creation_order) {
			unsigned crt_order_flags = H5P_CRT_ORDER_TRACKED | H5P_CRT_ORDER_INDEXED; // NOLINT(hicpp-signed-bitwise): Check is a false positive.
			auto cres = H5Pset_link_creation_order(plid, crt_order_flags);
			if (cres < 0) throw;  // HH_throw;
		}
		HH_hid_t pl(plid, Handles::Closers::CloseHDF5PropertyList::CloseP);
		return pl;
	}

	HH_hid_t GroupParameterPack::LinkCreationPListProperties::generateLinkCreationPlist() const {
		if (UseCustomLinkCreationPlist) return LinkCreationPlistCustom;
		hid_t plid = H5Pcreate(H5P_LINK_CREATE);
		HH_Expects(plid >= 0);
		if (CreateIntermediateGroups) {
			auto cres = H5Pset_create_intermediate_group(plid, 1);
			HH_Expects(cres >= 0);
		}
		HH_hid_t pl(plid, Handles::Closers::CloseHDF5PropertyList::CloseP);
		return pl;
	}

	GroupParameterPack::GroupParameterPack() = default;

	GroupParameterPack::GroupParameterPack(const AttributeParameterPack& a,
		HH_hid_t GroupAccessPlist) : atts(a), GroupAccessPlist(GroupAccessPlist) {}

	Group::Group() : Group(HH_hid_t{}) {}
	Group::Group(HH_hid_t hnd) : base(hnd), atts(hnd), dsets(hnd) { }
	Group::~Group() = default;

	HH_hid_t Group::get() const { return base; }

	H5G_info_t& Group::get_info(H5G_info_t& info) const {
		HH_Expects(isGroup());
		herr_t err = H5Gget_info(base(), &info);
		HH_Expects(err >= 0);
		return info;
	}

	bool Group::isGroup(HH_hid_t obj) {
		H5I_type_t typ = H5Iget_type(obj());
		if (typ == H5I_BADID) return false;
		return ((typ == H5I_GROUP) || (typ == H5I_FILE));
	}
	bool Group::isGroup() const { return isGroup(base); }
	std::vector<std::string> Group::list() const {
		std::vector<std::string> res;
		H5G_info_t info;
		herr_t e = H5Gget_info(base(), &info);
		HH_Expects(e >= 0);
		for (hsize_t i = 0; i < info.nlinks; ++i) {
			// Get the name
			ssize_t szName = H5Lget_name_by_idx(base(), ".", H5_INDEX_NAME, H5_ITER_NATIVE,
				i, NULL, 0, H5P_DEFAULT);
			HH_Expects(szName >= 0);
			std::vector<char> vName(szName + 1, '\0');
			H5Lget_name_by_idx(base(), ".", H5_INDEX_NAME, H5_ITER_NATIVE,
				i, vName.data(), szName + 1, H5P_DEFAULT);

			// Get the object and check the type
#if H5_VERSION_GE(1,12,0)
			H5O_info1_t oinfo;
			herr_t err = H5Oget_info_by_name1(base(), vName.data(), &oinfo, H5P_DEFAULT); // H5P_DEFAULT only, per docs.
#else
			H5O_info_t oinfo;
			herr_t err = H5Oget_info_by_name(base(), vName.data(), &oinfo, H5P_DEFAULT); // H5P_DEFAULT only, per docs.
#endif
			if (err < 0) continue;
			if (oinfo.type == H5O_type_t::H5O_TYPE_GROUP) res.emplace_back(std::string(vName.data()));
		}
		return res;
	}
	bool Group::exists(const std::string & name, HH_hid_t LinkAccessPlist) const
	{
		HH_Expects(isGroup());
		auto paths = HH::splitPaths(name);
		for (size_t i = 0; i < paths.size(); ++i) {
			auto p = HH::condensePaths(paths, 0, i + 1);
			htri_t linkExists = H5Lexists(base(), p.c_str(), LinkAccessPlist());
			if (linkExists < 0) throw;  // HH_throw;
			if (linkExists == 0) return false;
		}

		// Check that the object is a group
#if H5_VERSION_GE(1,12,0)
		H5O_info1_t obj_info;
		herr_t err = H5Oget_info_by_name1(base(), name.c_str(), &obj_info, LinkAccessPlist());
#else
		H5O_info_t obj_info;
		herr_t err = H5Oget_info_by_name(base(), name.c_str(), &obj_info, LinkAccessPlist());
#endif
		if (err < 0) throw;  // HH_throw;
		return (obj_info.type == H5O_TYPE_GROUP);
	}
	Group Group::create(const std::string & name, GroupParameterPack gp)
	{
		HH_Expects(isGroup());
		auto lcp = gp.linkCreationProperties.generateLinkCreationPlist();
		auto gcp = gp.groupCreationProperties.generateGroupCreationPlist();
		hid_t res = H5Gcreate(
			base(),
			name.c_str(),
			lcp(),
			gcp(),
			gp.GroupAccessPlist());
		HH_Expects(res >= 0);
		HH_hid_t h(res, Closers::CloseHDF5Group::CloseP);
		gp.atts.apply(h);
		return Group(h);
	}
	Group Group::open(const std::string & name, HH_hid_t GroupAccessPlist)
	{
		HH_Expects(isGroup());
		hid_t g = H5Gopen(base(), name.c_str(), GroupAccessPlist());
		HH_Expects(g >= 0);
		return Group(HH_hid_t(g, Closers::CloseHDF5Group::CloseP));
	}
	void Group::mount(
		const std::string & destination_groupname,
		HH_hid_t source_file,
		HH_hid_t FileMountPlist)
	{
		auto res = H5Fmount(base(), destination_groupname.c_str(),
			source_file(), FileMountPlist());
		HH_Expects(res >= 0);
	}
	void Group::unmount(const std::string & mountpoint)
	{
		auto res = H5Funmount(base(), mountpoint.c_str());
		HH_Expects(res >= 0);
	}
} // namespace HH

