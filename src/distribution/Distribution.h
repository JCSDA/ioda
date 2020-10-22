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
 *           The subclasses of this base class need to fill in the isMyRecord,
 *           sum, min and max methods with the appropriate function. The client 
 *           will use the isMyRecord method to determine what records to keep
 *           when reading in observations.
 *
 * \author Xin Zhang (JCSDA)
 */

/*! \fn bool isMyRecord(std::size_t RecNum)
 *  \brief The client will use the isMyRecord method to determine what records to 
 *         keep when reading in observations.
 *  \param RecNum Record Number       
 */

/*! \fn void sum(x)
 *  \brief sum x across processors and overwrite x with the result. The
 *  subclass will take care of not counting duplicate records across processors
 *  \param x the value to be summed across processes. 
 */

/*! \fn void min(x)
 *  \brief overwrites x with the minimum value of x across all processors
 */

/*! \fn void max(x)
 *  \brief overwrites x the maximum value of x across all processs
 */


class Distribution {
 public:
    explicit Distribution(const eckit::mpi::Comm & Comm);
    virtual ~Distribution() = 0;

    virtual bool isMyRecord(std::size_t RecNum) const = 0;

    virtual void sum(double &x) = 0;
    virtual void sum(int &x) = 0;
    virtual void sum(size_t &x) = 0;
    virtual void sum(std::vector<double> &x) = 0;
    virtual void sum(std::vector<size_t> &x) = 0;

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
