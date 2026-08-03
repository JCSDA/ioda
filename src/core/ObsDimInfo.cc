/*
 * (C) Copyright 2025 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/core/ObsDimInfo.h"

namespace ioda {

// -----------------------------------------------------------------------------
ObsDimInfo::ObsDimInfo() {
    // The following code needs to stay in sync with the ObsDimensionId enum object.
    // The entries are the standard dimension names according to the unified naming convention.
    std::string dimName = "Location";
    dim_id_name_[ObsDimensionId::Location] = dimName;
    dim_id_size_[ObsDimensionId::Location] = 0;
    dim_name_id_[dimName] = ObsDimensionId::Location;

    dimName = "Channel";
    dim_id_name_[ObsDimensionId::Channel] = dimName;
    dim_id_size_[ObsDimensionId::Channel] = 0;
    dim_name_id_[dimName] = ObsDimensionId::Channel;

    dimName = "Layer";
    dim_id_name_[ObsDimensionId::Layer] = dimName;
    dim_id_size_[ObsDimensionId::Layer] = 0;
    dim_name_id_[dimName] = ObsDimensionId::Layer;
}

ObsDimensionId ObsDimInfo::get_dim_id(const std::string & dimName) const {
    return dim_name_id_.at(dimName);
}

std::string ObsDimInfo::get_dim_name(const ObsDimensionId dimId) const {
    return dim_id_name_.at(dimId);
}

std::size_t ObsDimInfo::get_dim_size(const ObsDimensionId dimId) const {
    return dim_id_size_.at(dimId);
}

const ObsDimInfo::ChanNumToIndexMap & ObsDimInfo::getChanNumToIndexMap() const {
    return chan_num_to_index_;
}

void ObsDimInfo::set_dim_size(const ObsDimensionId dimId, std::size_t dimSize) {
    dim_id_size_.at(dimId) = dimSize;
}

void ObsDimInfo::setChanNumToIndexMap(const std::vector<int> chanNums) {
    // Walk through the vector and place the number to index mapping into
    // the map structure.
    for (size_t i = 0; i < chanNums.size(); ++i) {
        chan_num_to_index_[chanNums[i]] = i;
    }
}

}  // namespace ioda
