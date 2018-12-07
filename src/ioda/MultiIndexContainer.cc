/*
 * (C) Copyright 2017 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/MultiIndexContainer.h"

namespace ioda {
// -----------------------------------------------------------------------------
  ObsSpaceContainer::ObsSpaceContainer(const eckit::Configuration & config) {
    oops::Log::trace() << "ioda::ObsSpaceContainer Constructor starts " << std::endl;
  }
// -----------------------------------------------------------------------------
  ObsSpaceContainer::~ObsSpaceContainer() {
    oops::Log::trace() << "ioda::ObsSpaceContainer deconstructed " << std::endl;
  }
// -----------------------------------------------------------------------------
  void ObsSpaceContainer::CreateFromFile(const std::string & filename, const std::string & mode,
                                         const util::DateTime & bgn, const util::DateTime & end,
                                         const double & missingvalue,
                                         const eckit::mpi::Comm & commMPI) {
    oops::Log::trace() << "ioda::ObsSpaceContainer opening file: " << filename << std::endl;

    fileio_.reset(ioda::IodaIOfactory::Create(filename, mode, bgn, end, missingvalue, commMPI));
    nlocs_ = fileio_->nlocs();
    nvars_ = fileio_->nvars();
    vectors_.reserve(nvars_*10);

    oops::Log::trace() << "ioda::ObsSpaceContainer opening file ends " << std::endl;
  }
// -----------------------------------------------------------------------------

  void ObsSpaceContainer::read_var(const std::string & group, const std::string & name) {
    std::size_t vsize{nlocs_};
    std::string gname(group);
    std::string db_name(name);
    if (group.size() > 0)
       db_name = name + "@" + group;
    else
       gname = "GroupUndefined";

    // Allocate memory
    std::unique_ptr<boost::any[]> vect{new boost::any[vsize]};
    fileio_->ReadVar_any(db_name, vect.get());

    // Push to a smart vector to keep the memory alive
    vectors_.push_back(std::move(vect));
    std::size_t indx = vectors_.size() - 1;
    ASSERT(indx+1 <= nvars_*10);
    DataContainer.insert({gname, name, vsize, vectors_[indx]});
  }

// -----------------------------------------------------------------------------

  void ObsSpaceContainer::LoadData() {
    oops::Log::trace() << "ioda::ObsSpaceContainer loading data starts " << std::endl;
    for (auto iter = (fileio_->varlist())->begin(); iter != (fileio_->varlist())->end(); ++iter) {
      read_var(std::get<1>(*iter), std::get<0>(*iter));
    }
    oops::Log::trace() << "ioda::ObsSpaceContainer loading data ends " << std::endl;
  }

// -----------------------------------------------------------------------------

bool ObsSpaceContainer::has(const std::string & group, const std::string & name) const {
  auto var = DataContainer.find(boost::make_tuple(group, name));
  return (var != DataContainer.end());
}

// -----------------------------------------------------------------------------

void ObsSpaceContainer::print(std::ostream & os) const {
  auto & var = DataContainer.get<ObsSpaceContainer::by_name>();
  os << "ObsSpace Multi.Index Container for IODA" << "\n";
  for (auto iter = var.begin(); iter != var.end(); ++iter)
    os << iter->name << "@" << iter->group << "\n";
}

// -----------------------------------------------------------------------------

}  // namespace ioda
