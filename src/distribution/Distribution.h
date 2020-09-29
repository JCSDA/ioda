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
 *           The subclasses of this base class need to fill in the isMyRecord method
 *           with the appropriate function. The client will use the isMyRecord method
 *           to determine what records to keep when reading in observations.
 *
 * \author Xin Zhang (JCSDA)
 */
class Distribution {
 public:
    explicit Distribution(const eckit::mpi::Comm & Comm);
    virtual ~Distribution() = 0;

    virtual bool isMyRecord(std::size_t RecNum) const = 0;
    virtual bool isDistributed() const = 0;

    virtual void sum(double &x) = 0;
    virtual void sum(int &x) = 0;

    virtual void min(double &x) = 0;
    virtual void min(float &x) = 0;
    virtual void min(int &x) = 0;

    virtual void max(double &x) = 0;
    virtual void max(float &x) = 0;
    virtual void max(int &x) = 0;
 protected:
     /*! \brief Local MPI communicator */
     const eckit::mpi::Comm & comm_;
};

}  // namespace ioda

#endif  // DISTRIBUTION_DISTRIBUTION_H_
