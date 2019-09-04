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
/*!
 * \brief class for distributing obs across multiple process elements
 *
 * \details This Distribution class is a base class where various subclasses of
 *          this class define different methods for distributing obs.
 *
 *          The Distribution constructor sets the comm_ and gnlocs_ data members
 *          in this class, clears the indx_ and recnums_ vector data memebers,
 *          and zeros out the nlocs_ and nrecs_ data memebers. The constructors
 *          for each subclass need to call the Distribution constructor, but leave
 *          the indx_, recnums_, nlocs_ and nrecs_ data members alone. Rather, the
 *          distribution() method for each subclass is responsible for setting the 
 *          indx_, recnums_, nlocs_ and nrecs_ data members.
 *
 * \author Xin Zhang (JCSDA)
 */
class Distribution {
 public:
    Distribution(const eckit::mpi::Comm & Comm, const std::size_t Gnlocs);
    virtual ~Distribution() = 0;

     /*! \brief Calculate the local array index based on the MPI communicator */
     virtual void distribution() = 0;

     /*! \brief Return the index vector that indicates the distribution */
     const std::vector<std::size_t> & index() const {return indx_;}

     /*! \brief Return the record number associated with the index */
     const std::vector<std::size_t> & recnum() const {return recnums_;}

     /*! \brief Return number of locations in the distribution */
     std::size_t nlocs() const {return nlocs_;}

     /*! \brief Return number of records in the distribution */
     std::size_t nrecs() const {return nrecs_;}

 protected:
     /*! \brief Index of location array being assigned to this processing element */
     std::vector<std::size_t> indx_;

     /*! \brief Index of location array being assigned to this processing element */
     std::vector<std::size_t> recnums_;

     /*! \brief Total number of observation locations */
     const std::size_t gnlocs_;

     /*! \brief Number of observation locations in the distribution */
     std::size_t nlocs_;

     /*! \brief Number of observation records in the distribution */
     std::size_t nrecs_;

     /*! \brief Local MPI communicator */
     const eckit::mpi::Comm & comm_;
};

}  // namespace ioda

#endif  // DISTRIBUTION_DISTRIBUTION_H_
