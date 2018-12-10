/*
 * (C) Copyright 2017 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef DISTRIBUTION_DISTRIBUTION_H_
#define DISTRIBUTION_DISTRIBUTION_H_

#include <vector>

#include "eckit/mpi/Comm.h"

namespace ioda {

// ---------------------------------------------------------------------

class Distribution {
 public:
    virtual ~Distribution() = 0;

     /*! \brief Calculate the local array index based on the MPI communicator */
     virtual void distribution(const eckit::mpi::Comm &, const std::size_t gnlocs) = 0;

     /*! \brief Return the index */
     const std::vector<std::size_t> & index() const {return indx_;}

     /*! \brief Erase the item from indx_*/
     void erase(const std::size_t &);

     /*! \brief Rank of this processing element */
     const std::size_t size() const {return indx_.size();}

 protected:
     /*! \brief Index of location array being assigned to this processing element */
     std::vector<std::size_t> indx_;
};

}  // namespace ioda

#endif  // DISTRIBUTION_DISTRIBUTION_H_
