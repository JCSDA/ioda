/*
 * (C) Copyright 2017 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "database/ObsSpaceContainer.h"

namespace ioda {

// -----------------------------------------------------------------------------

  ObsSpaceContainer::ObsSpaceContainer() {
    oops::Log::trace() << "ioda::ObsSpaceContainer Constructor starts " << std::endl;
  }

// -----------------------------------------------------------------------------

  ObsSpaceContainer::~ObsSpaceContainer() {
    oops::Log::trace() << "ioda::ObsSpaceContainer deconstructed " << std::endl;
  }

// -----------------------------------------------------------------------------

void ObsSpaceContainer::StoreToDb(const std::string & GroupName, const std::string & VarName,
               const std::vector<std::size_t> & VarShape, const int * VarData) {
  StoreToDb_helper<int>(GroupName, VarName, VarShape, VarData);
}

void ObsSpaceContainer::StoreToDb(const std::string & GroupName, const std::string & VarName,
               const std::vector<std::size_t> & VarShape, const float * VarData) {
  StoreToDb_helper<float>(GroupName, VarName, VarShape, VarData);
}

void ObsSpaceContainer::StoreToDb(const std::string & GroupName, const std::string & VarName,
           const std::vector<std::size_t> & VarShape, const std::string * VarData) {
  StoreToDb_helper<std::string>(GroupName, VarName, VarShape, VarData);
}

void ObsSpaceContainer::StoreToDb(const std::string & GroupName, const std::string & VarName,
           const std::vector<std::size_t> & VarShape, const util::DateTime * VarData) {
  StoreToDb_helper<util::DateTime>(GroupName, VarName, VarShape, VarData);
}

template <typename DataType>
void ObsSpaceContainer::StoreToDb_helper(const std::string & GroupName,
          const std::string & VarName, const std::vector<std::size_t> & VarShape,
          const DataType * VarData) {
  const std::type_info & VarType = typeid(DataType);

  // Calculate the total number of elements
  std::size_t VarSize = 1;
  for (std::size_t i = 0; i < VarShape.size(); i++) {
    VarSize *= VarShape[i];
  }

  // Search for VarRecord with GroupName, VarName combination
  VarRecord_set::iterator Var = DataContainer.find(boost::make_tuple(GroupName, VarName));
  if (Var != DataContainer.end()) {
    // Found the required record in database
    // Check if attempting to update a read only VarRecord
    if (Var->mode.compare("r") == 0) {
      std::string ErrorMsg =
                  "ObsSpaceContainer::StoreInDb: trying to overwrite a read-only record : "
                  + VarName + " : " + GroupName;
      ABORT(ErrorMsg);
    }

    // Update the record
      for (std::size_t ii = 0; ii < VarSize; ++ii) {
        Var->data.get()[ii] = VarData[ii];
      }

  } else {
    // The required record is not in database, update the database
    std::unique_ptr<boost::any[]> vect{ new boost::any[VarSize] };

    for (std::size_t ii = 0; ii < VarSize; ++ii)
      vect.get()[ii] = VarData[ii];

      DataContainer.insert({GroupName, VarName, VarType, VarShape, VarSize, vect});
  }
}

// -----------------------------------------------------------------------------

void ObsSpaceContainer::LoadFromDb(const std::string & GroupName, const std::string & VarName,
               const std::vector<std::size_t> & VarShape, int * VarData) {
  LoadFromDb_helper<int>(GroupName, VarName, VarShape, VarData);
}

void ObsSpaceContainer::LoadFromDb(const std::string & GroupName, const std::string & VarName,
               const std::vector<std::size_t> & VarShape, float * VarData) {
  LoadFromDb_helper<float>(GroupName, VarName, VarShape, VarData);
}

void ObsSpaceContainer::LoadFromDb(const std::string & GroupName, const std::string & VarName,
           const std::vector<std::size_t> & VarShape, std::string * VarData) {
  LoadFromDb_helper<std::string>(GroupName, VarName, VarShape, VarData);
}

void ObsSpaceContainer::LoadFromDb(const std::string & GroupName, const std::string & VarName,
           const std::vector<std::size_t> & VarShape, util::DateTime * VarData) {
  LoadFromDb_helper<util::DateTime>(GroupName, VarName, VarShape, VarData);
}

template <typename DataType>
void ObsSpaceContainer::LoadFromDb_helper(const std::string & GroupName,
              const std::string & VarName, const std::vector<std::size_t> & VarShape,
              DataType * VarData) {
  if (has(GroupName, VarName)) {
    // Found the required record in the database
    VarRecord_set::iterator Var = DataContainer.find(boost::make_tuple(GroupName, VarName));

    // Calculate the total number of elements
    std::size_t VarSize = 1;
    for (std::size_t i = 0; i < VarShape.size(); i++) {
      VarSize *= VarShape[i];
    }

    // Copy the elements into the output
    for (std::size_t i = 0; i < VarSize; i++) {
      VarData[i] = boost::any_cast<DataType>(Var->data.get()[i]);
    }
  } else {
    // Required record is not in the database
    std::string ErrorMsg =
           "ObsSpaceContainer::LoadFromDb: " + VarName + " @ " + GroupName +" is not found";
    ABORT(ErrorMsg);
  }
}

// -----------------------------------------------------------------------------

ObsSpaceContainer::VarIter ObsSpaceContainer::var_iter_begin() {
  VarIndex & var_index_ = DataContainer.get<by_variable>();
  return var_index_.begin();
}

// -----------------------------------------------------------------------------

ObsSpaceContainer::VarIter ObsSpaceContainer::var_iter_end() {
  VarIndex & var_index_ = DataContainer.get<by_variable>();
  return var_index_.end();
}

// -----------------------------------------------------------------------------

std::string ObsSpaceContainer::var_iter_vname(ObsSpaceContainer::VarIter var_iter) {
  return var_iter->variable;
}

// -----------------------------------------------------------------------------

std::string ObsSpaceContainer::var_iter_gname(ObsSpaceContainer::VarIter var_iter) {
  return var_iter->group;
}

// -----------------------------------------------------------------------------

std::string ObsSpaceContainer::var_iter_mode(ObsSpaceContainer::VarIter var_iter) {
  return var_iter->mode;
}

// -----------------------------------------------------------------------------

const std::type_info & ObsSpaceContainer::var_iter_type(ObsSpaceContainer::VarIter var_iter) {
  return var_iter->type;
}

// -----------------------------------------------------------------------------

std::size_t ObsSpaceContainer::var_iter_size(ObsSpaceContainer::VarIter var_iter) {
  return var_iter->size;
}

// -----------------------------------------------------------------------------

std::vector<std::size_t> ObsSpaceContainer::var_iter_shape(ObsSpaceContainer::VarIter var_iter) {
  return var_iter->shape;
}

// -----------------------------------------------------------------------------

  bool ObsSpaceContainer::has(const std::string & group, const std::string & variable) const {
    if (variable == "datetime") {
      auto var0 = DataContainer.find(boost::make_tuple(group, "date"));
      auto var1 = DataContainer.find(boost::make_tuple(group, "time"));
      return (var0 != DataContainer.end() && var1 != DataContainer.end());
    } else {
      auto var = DataContainer.find(boost::make_tuple(group, variable));
      return (var != DataContainer.end());
    }
  }

// -----------------------------------------------------------------------------

  void ObsSpaceContainer::print(std::ostream & os) const {
    auto & var = DataContainer.get<ObsSpaceContainer::by_variable>();
    os << "ObsSpace Multi.Index Container for IODA" << "\n";
    for (auto iter = var.begin(); iter != var.end(); ++iter)
      os << iter->variable << " @ " << iter->group << "\n";
  }

// -----------------------------------------------------------------------------

}  // namespace ioda
