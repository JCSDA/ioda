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

namespace util {
class DateTime;
}

namespace ioda {

// ---------------------------------------------------------------------
/*!
 * \brief class for distributing obs across multiple process elements
 *
 * \details This Distribution class is a base class where various subclasses of
 *          this class define different methods for distributing obs.
 *
 *           The subclasses of this base class need to implement all pure virtual functions.
 *           The client will use the isMyRecord method to determine what records to keep
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
 *  \brief overwrites x with the maximum value of x across all processors
 */


class Distribution {
 public:
    explicit Distribution(const eckit::mpi::Comm & Comm);
    virtual ~Distribution();

    virtual bool isMyRecord(std::size_t RecNum) const = 0;

    virtual void sum(double &x) const = 0;
    virtual void sum(int &x) const = 0;
    virtual void sum(size_t &x) const = 0;
    virtual void sum(std::vector<double> &x) const = 0;
    virtual void sum(std::vector<size_t> &x) const = 0;

    virtual void min(double &x) const = 0;
    virtual void min(float &x) const = 0;
    virtual void min(int &x) const = 0;

    virtual void max(double &x) const = 0;
    virtual void max(float &x) const = 0;
    virtual void max(int &x) const = 0;

    /*!
     * \brief Gather observation data from all processes and deliver the combined data to
     * all processes.
     *
     * \param[inout] x
     *   On input: a vector of data associated with observations held by the calling process.
     *   On output: a concatenation of the vectors x passed by all calling processes, in the order
     *   of process ranks, with duplicates removed (i.e. if any observations are duplicated across
     *   multiple processes, the elements of x corresponding to these data are included only once).
     */
    virtual void allGatherv(std::vector<size_t> &x) const = 0;
    virtual void allGatherv(std::vector<int> &x) const = 0;
    virtual void allGatherv(std::vector<float> &x) const = 0;
    virtual void allGatherv(std::vector<double> &x) const = 0;
    virtual void allGatherv(std::vector<util::DateTime> &x) const = 0;
    virtual void allGatherv(std::vector<std::string> &x) const = 0;

    /*!
     * \brief Compute the exclusive scan (partial sums) of data on the calling processes.
     *
     * \param[inout] x
     *   On input: a number associated in some way with the observations held by the calling
     *   process. On output: the sum of the values of \p x sent by all processes of lower rank than
     *   the calling process, *excluding those that hold the same observations as the calling
     *   process*.
     */
    virtual void exclusiveScan(size_t &x) const = 0;

 protected:
     /*! \brief Local MPI communicator */
     const eckit::mpi::Comm & comm_;
};

}  // namespace ioda

#endif  // DISTRIBUTION_DISTRIBUTION_H_
