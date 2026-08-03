/*
 * (C) Copyright 2025 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#pragma once

#include <map>
#include <string>
#include <vector>

namespace ioda {
    /// \brief Enum type for obs dimension ids
    /// \details The first two dimension names for now are Location and Channel. This will
    /// likely expand in the future, so make sure that this enum class and the following
    /// initializer function stay in sync.
    enum class ObsDimensionId {
        Location,
        Channel,
        Layer
    };

    /// \brief Wrapper class that maps dimension ids to names.
    class ObsDimInfo {
     public:
        typedef std::map<int, int> ChanNumToIndexMap;

        ObsDimInfo();

        /// \brief return the standard id value for the given dimension name
        ObsDimensionId get_dim_id(const std::string & dimName) const;

        /// \brief return the dimension name for the given dimension id
        std::string get_dim_name(const ObsDimensionId dimId) const;

        /// \brief return the dimension size for the given dimension id
        std::size_t get_dim_size(const ObsDimensionId dimId) const;

        /// \brief set the dimension size for the given dimension id
        void set_dim_size(const ObsDimensionId dimId, std::size_t dimSize);

        /// \brief return a reference to the channel number to index map
        const ChanNumToIndexMap & getChanNumToIndexMap() const;

        /// \brief set the channel number to index map
        /// \param chanNums channel numbers in order of the channel dimension
        void setChanNumToIndexMap(const std::vector<int> chanNums);

     private:
        /// \brief map going from dim id to dim name
        std::map<ObsDimensionId, std::string> dim_id_name_;

        /// \brief map going from dim id to dim size
        std::map<ObsDimensionId, std::size_t> dim_id_size_;

        /// \brief map going from dim name to id
        std::map<std::string, ObsDimensionId> dim_name_id_;

        /// \brief map to go from channel number (not necessarily consecutive)
        ///        to channel index (consecutive, starting from zero).
        ChanNumToIndexMap chan_num_to_index_;
    };

}  // namespace ioda
