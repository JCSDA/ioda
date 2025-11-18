/*
 * (C) Copyright 2017-2025 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#pragma once

#include <vector>

namespace ioda {
    //-------------------------------------------------------------------------------------
    struct ObsSourceStats {
      ObsSourceStats() : nlocs(0), sourceNlocs(0), gNlocs(0),
                         gNlocsOutsideTimewindow(0), gNlocsRejectQc(0),
                         nrecs(0), locIndices(), recNums() {}
      /// \brief total number of locations from the input source (file or generator)
      std::size_t nlocs;

      /// \brief total number of locations from the input source (file or generator)
      std::size_t sourceNlocs;

      /// \brief total number of locations across all MPI tasks
      std::size_t gNlocs;

      /// \brief number of nlocs from the obs source that are outside the time window
      std::size_t gNlocsOutsideTimewindow;

      /// \brief number of nlocs from the obs source that are outside the time window
      std::size_t gNlocsRejectQc;

      /// \brief number of records
      std::size_t nrecs;

      /// \brief indexes of locations to extract from the input obs file
      std::vector<std::size_t> locIndices;

      /// \brief record numbers associated with the location indexes
      std::vector<std::size_t> recNums;
    };

}  // namespace ioda
