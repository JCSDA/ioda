/*
 * (C) Copyright 2017 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef DISTRIBUTION_DISTRIBUTION_H_
#define DISTRIBUTION_DISTRIBUTION_H_

#include <memory>
#include <vector>

#include "eckit/config/Configuration.h"
#include "eckit/exception/Exceptions.h"
#include "eckit/geometry/Point2.h"
#include "eckit/mpi/Comm.h"
#include "oops/util/missingValues.h"
#include "oops/util/TypeTraits.h"

namespace util {
class DateTime;
}

namespace ioda {

template <typename T>
class Accumulator;

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
 *           assignRecord() must be called before isMyRecord() can be called.
 *           computePatchLocs() should be called when all records have been assigned.
 *
 *           for distributions with observations duplicated on multiple PEs 
 *           (currently Inefficient and Halo) the following terminology/logic is used:
 *           the set of obs on each PE is called halo obs. (as in within a halo of some location)
 *           obs in the halo set can be duplicated across multiple PEs
 *           the subset of halo obs. (patch obs.) "belong" only to this PE and are used to 
 *           compute reduce operations without duplication. 
 *           patch obs form complete, non-overlapping partition of the global set of obs. 
 *
 * \author Xin Zhang (JCSDA)
 *
 * \see Functions defined in DistributionUtils.h.
 */
class Distribution {
 public:
    explicit Distribution(const eckit::mpi::Comm & Comm);
    virtual ~Distribution();

    /*!
     * \brief Returns true if the distribution assigns all records to all PEs, false otherwise.
     */
    virtual bool isIdentity() const { return false; }

    /*!
     * \brief Returns true if the distribution does not assign any record to more than one PE,
     * false otherwise.
     */
    virtual bool isNonoverlapping() const { return false; }

    /*!
     * \brief If the record \p RecNum has not yet been assigned to a PE, assigns it to the
     * appropriate PE. Informs the distribution that location \p LocNum belongs to this record.
     *
     * \param RecNum Record containing the location \p LocNum.
     * \param LocNum (Global) location index.
     * \param point Latitude and longitude of this location.
     */
    virtual void assignRecord(const std::size_t RecNum, const std::size_t LocNum,
                              const eckit::geometry::Point2 & point) {}

    /*!
     * \brief Returns true if record \p RecNum has been assigned to the calling PE during a
     * previous call to assignRecord().
     *
     * Clients can use this function to determine which records to keep when reading in
     * observations.
     */
    virtual bool isMyRecord(std::size_t RecNum) const = 0;

    /*!
     * \brief If necessary, identifies locations of "patch obs", i.e. locations belonging to
     * records owned by this PE.
     *
     * This function must be called when all records have been assigned, and in particular
     * before any calls to the createAccumulator() and globalUniqueConsecutiveLocationIndex()
     * member functions or the global functions and dotProduct(), globalNumNonMissingObs().
     *
     * \param nglocs Total number of global locations.
     */
    virtual void computePatchLocs(const std::size_t nglocs) {}

    /*!
     * \brief Sets each element of the provided vector to true if the corresponding location is a
     * "patch obs", i.e. it belongs to a record owned by this PE, and to false otherwise.
     *
     * \param isPatchObs Preallocated vector of as many elements as there are locations on this PE.
     */
    virtual void patchObs(std::vector<bool> & isPatchObs) const = 0;

    /*!
     * \brief Calculates the global minimum (over all locations on all PEs) of a
     * location-dependent quantity.
     *
     * \param[inout] x
     *   On input, the local minimum (over all locations on the current PE) of a location-dependent
     *   quantity. On output, its global minimum.
     */
    virtual void min(int & x) const = 0;
    virtual void min(std::size_t & x) const = 0;
    virtual void min(float & x) const = 0;
    virtual void min(double & x) const = 0;

    /*!
     * \brief Calculates the global minima (over all locations on all PEs) of multiple
     * location-dependent quantities.
     *
     * \param[inout] x
     *   On input, each element of `x` should be the local minimum (over all locations on the
     *   current PE) of a location-dependent quantity. On output, that element will be set to the
     *   global minimum of that quantity.
     */
    virtual void min(std::vector<int> & x) const = 0;
    virtual void min(std::vector<std::size_t> & x) const = 0;
    virtual void min(std::vector<float> & x) const = 0;
    virtual void min(std::vector<double> & x) const = 0;

    /*!
     * \brief Calculates the global maximum (over all locations on all PEs) of a
     * location-dependent quantity.
     *
     * \param[inout] x
     *   On input, the local maximum (over all locations on the current PE) of a location-dependent
     *   quantity. On output, its global maximum.
     */
    virtual void max(int & x) const = 0;
    virtual void max(std::size_t & x) const = 0;
    virtual void max(float & x) const = 0;
    virtual void max(double & x) const = 0;

    /*!
     * \brief Calculates the global maxima (over all locations on all PEs) of multiple
     * location-dependent quantities.
     *
     * \param[inout] x
     *   On input, each element of `x` should be the local maximum (over all locations on the
     *   current PE) of a location-dependent quantity. On output, that element will be set to the
     *   global maximum of that quantity.
     */
    virtual void max(std::vector<int> & x) const = 0;
    virtual void max(std::vector<std::size_t> & x) const = 0;
    virtual void max(std::vector<float> & x) const = 0;
    virtual void max(std::vector<double> & x) const = 0;

    /*!
     * \brief Create an object that can be used to calculate the sum of a location-dependent
     * quantity over locations held on all PEs, each taken into account only once even if it's
     * held on multiple PEs.
     *
     * \tparam T
     *   Must be either `int`, `size_t`, `float` or `double`.
     * \param init
     *   Used only to select the right overload -- the value is ignored.
     */
    template <typename T>
    std::unique_ptr<Accumulator<T>> createAccumulator() const {
        static_assert(util::any_is_same<T, int, std::size_t, float, double>::value,
                      "in the call to createAccumulator<T>(), "
                      "T must be int, size_t, float or double");
        return createAccumulatorImpl(T());
    }

    /*!
     * \brief Create an object that can be used to calculate the sums of `n`
     * location-dependent quantities over locations held on all PEs, each taken into account only
     * once even if it's held on multiple PEs.
     *
     * \tparam T
     *   Must be either `int`, `size_t`, `float` or `double`.
     * \param n
     *   The number of quantities to be summed up.
     */
    template <typename T>
    std::unique_ptr<Accumulator<std::vector<T>>> createAccumulator(std::size_t n) const {
        static_assert(util::any_is_same<T, int, std::size_t, float, double>::value,
                      "in the call to createAccumulator<T>(size_t n), "
                      "T must be int, size_t, float or double");
        return createAccumulatorImpl(std::vector<T>(n));
    }

    /*!
     * \brief Gather observation data from all processes and deliver the combined data to
     * all processes.
     *
     * \param[inout] x
     *   On input: a vector whose ith element is associated with the ith observation held by the
     *   calling process.
     *   On output: a concatenation of the vectors `x` passed by all calling processes, with
     *   duplicates removed (i.e. if any observations are duplicated across multiple processes, the
     *   elements of `x` corresponding to these data are included only once).
     */
    virtual void allGatherv(std::vector<size_t> &x) const = 0;
    virtual void allGatherv(std::vector<int> &x) const = 0;
    virtual void allGatherv(std::vector<float> &x) const = 0;
    virtual void allGatherv(std::vector<double> &x) const = 0;
    virtual void allGatherv(std::vector<util::DateTime> &x) const = 0;
    virtual void allGatherv(std::vector<std::string> &x) const = 0;

    /*!
     * \brief Map the index of a location held on the calling process to the index of the
     * corresponding element of any vector produced by allGatherv().
     */
    virtual size_t globalUniqueConsecutiveLocationIndex(size_t loc) const = 0;

    // return the name of the distribution
    virtual std::string name() const = 0;

    /// Deprecated accessor to MPI communicator (added temporarily, to be removed soon, May 2021)
    const eckit::mpi::Comm & comm() const {return comm_;}

 private:
  /*!
   * \brief Create an object that can be used to calculate the sum of a location-dependent
   * quantity over locations held on all PEs, each taken into account only once even if it's
   * held on multiple PEs.
   *
   * \param init
   *   Used only to select the right overload -- the value is ignored.
   */
  virtual std::unique_ptr<Accumulator<int>>
      createAccumulatorImpl(int init) const = 0;
  virtual std::unique_ptr<Accumulator<std::size_t>>
      createAccumulatorImpl(std::size_t init) const = 0;
  virtual std::unique_ptr<Accumulator<float>>
      createAccumulatorImpl(float init) const = 0;
  virtual std::unique_ptr<Accumulator<double>>
      createAccumulatorImpl(double init) const = 0;

  /*!
   * \brief Create an object that can be used to calculate the sums of multiple
   * location-dependent quantities over locations held on all PEs, each taken into account only
   * once even if it's held on multiple PEs.
   *
   * \param init
   *   A vector of as many elements as there are sums to calculate. The values of these elements
   *   are ignored.
   */
  virtual std::unique_ptr<Accumulator<std::vector<int>>>
      createAccumulatorImpl(const std::vector<int> & init) const = 0;
  virtual std::unique_ptr<Accumulator<std::vector<std::size_t>>>
      createAccumulatorImpl(const std::vector<std::size_t> & init) const = 0;
  virtual std::unique_ptr<Accumulator<std::vector<float>>>
      createAccumulatorImpl(const std::vector<float> & init) const = 0;
  virtual std::unique_ptr<Accumulator<std::vector<double>>>
      createAccumulatorImpl(const std::vector<double> & init) const = 0;

 protected:
     /*! \brief Local MPI communicator */
     const eckit::mpi::Comm & comm_;
};

}  // namespace ioda

#endif  // DISTRIBUTION_DISTRIBUTION_H_
