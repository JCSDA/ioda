/*
 * (C) Crown copyright 2021, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef DISTRIBUTION_ACCUMULATOR_H_
#define DISTRIBUTION_ACCUMULATOR_H_

#include <vector>

namespace ioda {

/// \brief Calculates the sum of a location-dependent quantity of type `T` over locations held on
/// all PEs, each taken into account only once even if it's held on multiple PEs.
///
/// The intended usage is as follows:
/// 1. Create an Accumulator by calling the `createAccumulator()` method of a Distribution.
/// 2. Iterate over locations held on the current PE and call addTerm() for each location making a
///    non-zero contribution to the sum.
/// 3. Call computeResult() to calculate the global sum (over all PEs).
///
/// Subclasses need to implement addTerm() and computeResult() in such a way that contributions made
/// by locations held on multiple PEs are included only once in the global sum.
template <typename T>
class Accumulator {
 public:
    virtual ~Accumulator() {}

    /// \brief Increment the sum with the contribution `term` of location `loc` held on the
    /// current PE.
    virtual void addTerm(std::size_t loc, const T & term) = 0;

    /// \brief Return the sum of contributions associated with locations held on all PEs
    /// (each taken into account only once).
    virtual T computeResult() const = 0;
};

/// \brief Calculates the sums of multiple location-dependent quantities of type `T` over locations
/// held on all PEs, each taken into account only once even if it's held on multiple PEs.
///
/// The intended usage is the same as of the primary template, except that this specialization for
/// `std::vector<T>` provides two overloads of addTerm(). Use whichever is more convenient.
template <typename T>
class Accumulator<std::vector<T>> {
 public:
    virtual ~Accumulator() {}

    /// \brief Increment each sum with the contribution of location `loc` (held on the current PE)
    /// taken from the corresponding element of `term`.
    virtual void addTerm(std::size_t loc, const std::vector<T> & term) = 0;

    /// \brief Increment the `i`th sum with the contribution `term` of location `loc` held on the
    /// current PE.
    virtual void addTerm(std::size_t loc, std::size_t i, const T & term) = 0;

    /// \brief Return the sums of contributions associated with locations held on all
    /// PEs (each taken into account only once).
    virtual std::vector<T> computeResult() const = 0;
};

}  // namespace ioda

#endif  // DISTRIBUTION_ACCUMULATOR_H_
