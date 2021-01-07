/*
 * (C) Copyright 2017-2020 Ryan Honeyager (ryan@honeyager.info)
 *
 * This software is licensed under the terms of the BSD 2-Clause License.
 * For details, see the LICENSE file.
 */
#include <stdexcept>
#include "HH/Datasets.hpp"

namespace HH {
	using namespace HH::Handles;
	using namespace HH::Types;
	using std::initializer_list;

	Dataset::Dataset(HH_hid_t hnd_dset) : dset(hnd_dset), atts(hnd_dset) {}
	Dataset::~Dataset() = default;
	HH_hid_t Dataset::get() const { return dset; }
	bool Dataset::isDataset(HH_hid_t obj) {
		H5I_type_t typ = H5Iget_type(obj());
		if (typ == H5I_BADID) return false;
		return (typ == H5I_DATASET);
	}
	HH_NODISCARD HH_hid_t Dataset::getType() const
	{
		HH_Expects(isDataset());
		return HH_hid_t(H5Dget_type(dset()), Closers::CloseHDF5Datatype::CloseP);
	}
	HH_NODISCARD HH_hid_t Dataset::getSpace() const
	{
		HH_Expects(isDataset());
		return HH_hid_t(H5Dget_space(dset()), Closers::CloseHDF5Dataspace::CloseP);
	}
	Dataset::Dimensions Dataset::getDimensions() const
	{
		HH_Expects(isDataset());
		std::vector<hsize_t> dimsCur, dimsMax;
		auto space = getSpace();
		if (H5Sis_simple(space()) <= 0) throw;  // HH_throw;
		auto numPoints = gsl::narrow<hsize_t>(H5Sget_simple_extent_npoints(space()));
		int dimensionality = H5Sget_simple_extent_ndims(space());
		if (dimensionality < 0) throw;  // HH_throw;
		dimsCur.resize(dimensionality);
		dimsMax.resize(dimensionality);
		int err = H5Sget_simple_extent_dims(space(), dimsCur.data(), dimsMax.data());
		if (err < 0) throw;  // HH_throw;

		return Dimensions(dimsCur, dimsMax, dimsCur.size(), numPoints);
	}
	Dataset Dataset::attachDimensionScale(unsigned int DimensionNumber, const Dataset& scale)
	{
		HH_Expects(isDataset());
		HH_Expects(isDataset(scale.get()));
		const herr_t res = H5DSattach_scale(dset(), scale.dset(), DimensionNumber);
		if (res != 0) throw;  // HH_throw;
		return *this;
	}
	Dataset Dataset::detachDimensionScale(unsigned int DimensionNumber, const Dataset & scale)
	{
		HH_Expects(isDataset());
		HH_Expects(isDataset(scale.get()));
		const herr_t res = H5DSdetach_scale(dset(), scale.dset(), DimensionNumber);
		if (res != 0) throw;  // HH_throw;
		return *this;
	}
	Dataset Dataset::setDims(std::initializer_list<Dataset> dims) {
		unsigned int i = 0;
		for (const auto *it = dims.begin(); it != dims.end(); ++it, ++i) {
			attachDimensionScale(i, *it);
		}
		return *this;
	}
	Dataset Dataset::setDims(const Dataset & dims) {
		attachDimensionScale(0, dims);
		return *this;
	}
	Dataset Dataset::setDims(const Dataset & dim1, const Dataset & dim2) {
		attachDimensionScale(0, dim1);
		attachDimensionScale(1, dim2);
		return *this;
	}
	Dataset Dataset::setDims(const Dataset & dim1, const Dataset & dim2, const Dataset & dim3) {
		attachDimensionScale(0, dim1);
		attachDimensionScale(1, dim2);
		attachDimensionScale(2, dim3);
		return *this;
	}
	bool Dataset::isDimensionScale() const {
		HH_Expects(isDataset());
		const htri_t res = H5DSis_scale(dset());
		if (res < 0) throw;  // HH_throw;
		return (res > 0);
	}
	Dataset Dataset::setIsDimensionScale(const std::string& dimensionScaleName) {
		HH_Expects(isDataset());
		const htri_t res = H5DSset_scale(dset(), dimensionScaleName.c_str());
		if (res != 0) throw;  // HH_throw;
		return *this;
	}
	Dataset Dataset::setDimensionScaleAxisLabel(unsigned int DimensionNumber, const std::string& label)
	{
		HH_Expects(isDataset());
		const htri_t res = H5DSset_label(dset(), DimensionNumber, label.c_str());
		if (res != 0) throw;  // HH_throw;
		return *this;
	}
	std::string Dataset::getDimensionScaleAxisLabel(unsigned int DimensionNumber) const
	{
		HH_Expects(isDataset());
		constexpr size_t max_label_size = 1000;
		std::array<char, max_label_size> label; // NOLINT: See next line.
		label.fill('\0');
		const ssize_t res = H5DSget_label(dset(), DimensionNumber, label.data(), max_label_size);
		// res is the size of the label. The HDF5 documentation does not include whether the label is null-terminated,
		// so I am terminating it manually.
		if (res < 0) throw;  // HH_throw;
		label[max_label_size - 1] = '\0';
		return std::string(label.data());
	}
	Dataset Dataset::getDimensionScaleAxisLabel(unsigned int DimensionNumber, std::string& res) const {
		HH_Expects(isDataset());
		res = getDimensionScaleAxisLabel(DimensionNumber);
		return *this;
	}
	std::string Dataset::getDimensionScaleName() const
	{
		HH_Expects(isDataset());
		constexpr size_t max_label_size = 1000;
		std::array<char, max_label_size> label; // NOLINT: See next line.
		label.fill('\0');
		const ssize_t res = H5DSget_scale_name(dset(), label.data(), max_label_size);
		if (res < 0) throw;  // HH_throw;
		// res is the size of the label. The HDF5 documentation does not include whether the label is null-terminated,
		// so I am terminating it manually.
		label[max_label_size - 1] = '\0';
		return std::string(label.data());
	}
	Dataset Dataset::getDimensionScaleName(std::string& res) const {
		res = getDimensionScaleName();
		return *this;
	}
	bool Dataset::isDimensionScaleAttached(const Dataset& scale, unsigned int DimensionNumber) const {
		HH_Expects(scale.isDataset());
		HH_Expects(isDataset());
		auto ret = H5DSis_attached(dset(), scale.get()(), DimensionNumber);
		if (ret < 0) throw;  // HH_throw;
		return (ret > 0);
	}

	std::pair<bool, bool> isFilteravailable(H5Z_filter_t filt) {
		unsigned int filter_config = 0;
		htri_t avl = H5Zfilter_avail(filt);
		if (avl <= 0) return std::make_pair(false, false);
		herr_t err = H5Zget_filter_info(filt, &filter_config);
		if (err < 0) throw;  // HH_throw;
		bool compress = false;
		bool decompress = false;
		if (filter_config & H5Z_FILTER_CONFIG_ENCODE_ENABLED) compress = true; // NOLINT(hicpp-signed-bitwise): Check is a false positive.
		if (filter_config & H5Z_FILTER_CONFIG_DECODE_ENABLED) decompress = true; // NOLINT(hicpp-signed-bitwise): Check is a false positive.
		return std::make_pair(compress, decompress);
	}

	Filters::Filters(HH_hid_t newbase) : pl(newbase) {}
	Filters::~Filters() = default;
	std::vector<Filters::filter_info> Filters::get() const {
		int nfilts = H5Pget_nfilters(pl());
		if (nfilts < 0) throw;  // HH_throw;
		std::vector<filter_info> res;
		for (int i = 0; i < nfilts; ++i) {
			filter_info obj;
			size_t cd_nelems = 0;
			obj.id = H5Pget_filter2(pl(), i, &(obj.flags), &cd_nelems, nullptr, 0, nullptr, nullptr);
			obj.cd_values.resize(cd_nelems);
			H5Pget_filter2(pl(), i, &(obj.flags), &cd_nelems, obj.cd_values.data(), 0, nullptr, nullptr);

			res.push_back(obj);
		}
		return res;
	}
	void Filters::append(const std::vector<filter_info>& filters) {
		for (const auto& f : filters) {
			herr_t res = H5Pset_filter(pl(), f.id, f.flags, f.cd_values.size(), f.cd_values.data());
			if (res < 0) throw;  // HH_throw;
		}
	}
	void Filters::set(const std::vector<filter_info>& filters) {
		if (H5Premove_filter(pl(), H5Z_FILTER_ALL) < 0) throw;  // HH_throw;
		append(filters);
	}
	void Filters::clear() {
		if (H5Premove_filter(pl(), H5Z_FILTER_ALL) < 0) throw;  // HH_throw;
	}
	bool Filters::has(H5Z_filter_t id) const {
		auto fi = get();
		auto res = std::find_if(fi.cbegin(), fi.cend(), [&id](const filter_info & f) {return f.id == id; });
		return (res != fi.cend());
	}
	Filters::FILTER_T Filters::getType(const filter_info& it) {
		if ((it.id == H5Z_FILTER_SHUFFLE)) return FILTER_T::SHUFFLE;
		if ((it.id == H5Z_FILTER_DEFLATE)) return FILTER_T::COMPRESSION;
		if ((it.id == H5Z_FILTER_SZIP)) return FILTER_T::COMPRESSION;
		if ((it.id == H5Z_FILTER_NBIT)) return FILTER_T::COMPRESSION;
		if ((it.id == H5Z_FILTER_SCALEOFFSET)) return FILTER_T::COMPRESSION;
		return FILTER_T::OTHER;
	}
	bool Filters::isA(const filter_info & it, Filters::FILTER_T typ) {
		auto ft = getType(it);
		return (ft == typ);
	}
	void Filters::appendOfType(const std::vector<filter_info> & filters, FILTER_T typ) {
		for (auto it = filters.cbegin(); it != filters.cend(); ++it)
		{
			if (isA(*it, typ)) {
				herr_t res = H5Pset_filter(pl(), it->id, it->flags, it->cd_values.size(), it->cd_values.data());
				if (res < 0) throw;  // HH_throw;
			}
		}
	}
	void Filters::removeOfType(FILTER_T typ) {
		auto fils = get();
		clear();
		for (auto it = fils.cbegin(); it != fils.cend(); ++it)
		{
			if (!isA(*it, typ)) {
				herr_t res = H5Pset_filter(pl(), it->id, it->flags, it->cd_values.size(), it->cd_values.data());
				if (res < 0) throw;  // HH_throw;
			}
		}
	}

	void Filters::setShuffle() {
		if (has(H5Z_FILTER_SHUFFLE)) return;
		auto fils = get();
		clear();
		appendOfType(fils, FILTER_T::SCALE);
		if (0 > H5Pset_shuffle(pl())) throw;  // HH_throw; // Bit shuffling.
		appendOfType(fils, FILTER_T::COMPRESSION);
		appendOfType(fils, FILTER_T::OTHER);
	}
	void Filters::setSZIP(unsigned int optm, unsigned int ppb) {
		if (has(H5Z_FILTER_SZIP)) return;
		auto fils = get();
		clear();
		appendOfType(fils, FILTER_T::SCALE);
		appendOfType(fils, FILTER_T::SHUFFLE);

		//unsigned int optm = H5_SZIP_EC_OPTION_MASK;
		//unsigned int ppb = 16;
		//if (pixels_per_block.has_value()) ppb = pixels_per_block.value();

		if (0 > H5Pset_szip(pl(), optm, ppb)) throw;  // HH_throw;
		appendOfType(fils, FILTER_T::OTHER);
	}
	void Filters::setGZIP(unsigned int level) {
		auto fils = get();
		clear();
		appendOfType(fils, FILTER_T::SCALE);
		appendOfType(fils, FILTER_T::SHUFFLE);
		if (0 > H5Pset_deflate(pl(), level)) throw;  // HH_throw;
		appendOfType(fils, FILTER_T::OTHER);
	}
	void Filters::setScaleOffset(H5Z_SO_scale_type_t scale_type, int scale_factor)
	{
		auto fils = get();
		clear();
		if (0 > H5Pset_scaleoffset(pl(), scale_type, scale_factor)) throw;  // HH_throw; // Bit shuffling.
		appendOfType(fils, FILTER_T::SHUFFLE);
		appendOfType(fils, FILTER_T::COMPRESSION);
		appendOfType(fils, FILTER_T::OTHER);
	}

	HH_hid_t DatasetParameterPack::DatasetCreationPListProperties::generate(
		const std::vector<hsize_t>& chunkingBlockSize) const
	{
		hid_t plid = H5Pcreate(H5P_DATASET_CREATE);
		if (plid < 0) throw;  // HH_throw;
		HH_hid_t pl(plid, Handles::Closers::CloseHDF5PropertyList::CloseP);

		Filters filters(pl);
		if (scale) filters.setScaleOffset(scale_type, scale_factor);
		if (shuffle) filters.setShuffle();
		if (gzip) filters.setGZIP(gzip_level);
		if (szip) filters.setSZIP(szip_options, szip_PixelsPerBlock);
		if (chunk) {
			HH_Expects(0 <= H5Pset_chunk(pl(),
				static_cast<int>(chunkingBlockSize.size()),
				chunkingBlockSize.data()));
		}
		if (hasFillValue) {
			HH_Expects(0 <= H5Pset_fill_value(pl(), fillValue_type(), &(fillValue)));
		}

		return pl;
	}


	DatasetParameterPack& DatasetParameterPack::attachDimensionScale(unsigned int DimensionNumber, const Dataset& scale)
	{
		_dimsToAttach.emplace_back(std::make_pair(DimensionNumber, scale));
		return *this;
	}
	DatasetParameterPack& DatasetParameterPack::setDims(std::initializer_list<Dataset> dims) {
		_dimsToAttach.clear();
		unsigned int i = 0;
		for (const auto *it = dims.begin(); it != dims.end(); ++it, ++i) {
			attachDimensionScale(i, *it);
		}
		return *this;
	}
	DatasetParameterPack& DatasetParameterPack::setDims(const Dataset& dims) {
		_dimsToAttach.clear();
		attachDimensionScale(0, dims);
		return *this;
	}
	DatasetParameterPack& DatasetParameterPack::setDims(const Dataset& dim1, const Dataset& dim2) {
		_dimsToAttach.clear();
		attachDimensionScale(0, dim1);
		attachDimensionScale(1, dim2);
		return *this;
	}
	DatasetParameterPack& DatasetParameterPack::setDims(const Dataset& dim1, const Dataset& dim2, const Dataset& dim3) {
		_dimsToAttach.clear();
		attachDimensionScale(0, dim1);
		attachDimensionScale(1, dim2);
		attachDimensionScale(2, dim3);
		return *this;
	}

	Dataset DatasetParameterPack::apply(HH::HH_hid_t h) const {
		HH::Dataset d(h);

		for (const auto& ndims : _dimsToAttach)
			d.attachDimensionScale(ndims.first, ndims.second);
		atts.apply(h);
		return d;
	}

	HH_hid_t DatasetParameterPack::generateDatasetCreationPlist(const std::vector<hsize_t> & dims) const
	{
		if (UseCustomDatasetCreationPlist) return DatasetCreationPlistCustom;

		if (!customChunkSizes.empty())
			return datasetCreationProperties.generate(customChunkSizes);
		std::vector<hsize_t> chunk_sizes;
		HH_Expects(fChunkingStrategy(dims, chunk_sizes));
		return datasetCreationProperties.generate(chunk_sizes);
	}

	DatasetParameterPack::DatasetParameterPack() = default;
	DatasetParameterPack::DatasetParameterPack(const AttributeParameterPack & a,
		HH_hid_t LinkCreationPlist,
		HH_hid_t DatasetAccessPlist) : atts(a), LinkCreationPlist(LinkCreationPlist),
		//DatasetCreationPlist(DatasetCreationPlist), 
		DatasetAccessPlist(DatasetAccessPlist)
	{}


	Has_Datasets::Has_Datasets(HH_hid_t obj) : base(obj) {  }
	Has_Datasets::~Has_Datasets() = default;
	bool Has_Datasets::exists(const std::string& dsetname, HH_hid_t LinkAccessPlist) const {
		auto paths = HH::splitPaths(dsetname);
		for (size_t i = 0; i < paths.size(); ++i) {
			auto p = HH::condensePaths(paths, 0, i + 1);
			htri_t linkExists = H5Lexists(base(), p.c_str(), LinkAccessPlist());
			if (linkExists < 0) throw;  // HH_throw;
			if (linkExists == 0) return false;
		}
#if H5_VERSION_GE(1,12,0)
		H5O_info1_t oinfo;
		herr_t err = H5Oget_info_by_name1(base(), dsetname.c_str(), &oinfo, H5P_DEFAULT); // H5P_DEFAULT only, per docs.
#else
		H5O_info_t oinfo;
		herr_t err = H5Oget_info_by_name(base(), dsetname.c_str(), &oinfo, H5P_DEFAULT); // H5P_DEFAULT only, per docs.
#endif
		if (err < 0) throw;  // HH_throw;
		return (oinfo.type == H5O_type_t::H5O_TYPE_DATASET);
	}
	Dataset Has_Datasets::open(const std::string & dsetname, HH_hid_t DatasetAccessPlist) const
	{
		hid_t dsetid = H5Dopen(base(), dsetname.c_str(), DatasetAccessPlist());
		if (dsetid < 0) throw std::logic_error("Cannot open dataset");  // HH_throw.add("Reason", "Cannot open dataset").add("Name", dsetname);
		return Dataset(HH_hid_t(dsetid, Closers::CloseHDF5Dataset::CloseP));
	}
	Dataset Has_Datasets::operator[](const std::string & dsetname) const {
		return open(dsetname);
	}
	std::vector<std::string> Has_Datasets::list() const {
		std::vector<std::string> res;
		H5G_info_t info;
		herr_t e = H5Gget_info(base(), &info);
		if (e < 0) throw;  // HH_throw;
		res.reserve(gsl::narrow<size_t>(info.nlinks));
		for (hsize_t i = 0; i < info.nlinks; ++i) {
			// Get the name
			ssize_t szName = H5Lget_name_by_idx(base(), ".",
				H5_INDEX_NAME, H5_ITER_NATIVE, i, NULL, 0, H5P_DEFAULT);
			if (szName < 0) throw;  // HH_throw;
			std::vector<char> vName(szName + 1, '\0');
			H5Lget_name_by_idx(base(), ".", H5_INDEX_NAME, H5_ITER_NATIVE,
				i, vName.data(), szName + 1, H5P_DEFAULT);

			// Get the object and check the type
#if H5_VERSION_GE(1,12,0)
			H5O_info1_t oinfo;
			herr_t err = H5Oget_info_by_idx1(base(), ".",
				H5_INDEX_NAME, H5_ITER_NATIVE, i, &oinfo, H5P_DEFAULT);
			//herr_t err = H5Oget_info_by_name1(base(), vName.data(), &oinfo, H5P_DEFAULT); // H5P_DEFAULT only, per docs.
#else
			H5O_info_t oinfo;
			herr_t err = H5Oget_info_by_idx(base(), ".",
				H5_INDEX_NAME, H5_ITER_NATIVE, i, &oinfo, H5P_DEFAULT);
			//herr_t err = H5Oget_info_by_name(base(), vName.data(), &oinfo, H5P_DEFAULT); // H5P_DEFAULT only, per docs.
#endif
			if (err < 0) continue;
			if (oinfo.type == H5O_type_t::H5O_TYPE_DATASET) res.emplace_back(std::string(vName.data()));
		}
		return res;
	}
	std::map<std::string, Dataset> Has_Datasets::openAll() const {
		auto ls = list();
		std::map<std::string, Dataset> res;
		for (const auto& l : ls) {
			res[l] = open(l);
		}
		return res;
	}
	void Has_Datasets::remove(const std::string& name) {
		auto ret = H5Ldelete(base(), name.c_str(), H5P_DEFAULT);
		if (ret < 0) throw std::logic_error("Failed to remove link to dataset.");  // HH_throw.add("Reason", "Failed to remove link to dataset.")
			//.add("name", name)
			//.add("Return code", ret);
	}
} // namespace HH

