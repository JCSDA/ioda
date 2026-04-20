/*
 * (C) Copyright 2026-2026 UCAR.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef IODA_OBSITERATOR_H_
#define IODA_OBSITERATOR_H_

#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include "oops/util/Printable.h"
#include "oops/util/ObjectCounter.h"

namespace eckit::geometry {
  class Point3;
}

namespace ioda {

class ObsSpace;

class ObsIterator: public util::Printable, private util::ObjectCounter<ObsIterator> {
 public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = eckit::geometry::Point3;
  using difference_type = std::ptrdiff_t;
  using pointer = eckit::geometry::Point3*;
  using reference = eckit::geometry::Point3&;

  static std::string classname() {return "ioda::ObsIterator";}

  ObsIterator(const ObsIterator &) = default;
  ObsIterator& operator=(const ObsIterator &) = default;

  bool operator==(const ObsIterator &) const;
  bool operator!=(const ObsIterator &) const;
  eckit::geometry::Point3 operator*() const;
  ObsIterator& operator++();

 private:
  ObsIterator(const ObsSpace & obsSpace, size_t index);
  void print(std::ostream & os) const override;

  const ObsSpace& obsSpace_;
  mutable std::shared_ptr<std::vector<float>> lats_;
  mutable std::shared_ptr<std::vector<float>> lons_;
  size_t obIndex_;

  friend class ObsSpace;
};

}  // namespace ioda

#endif  // IODA_OBSITERATOR_H_
