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

  void ObsSpaceContainer::inquire(const std::string & group, const std::string & variable,
                                  const std::size_t vsize, double vdata[]) const {
    // Obtain the missing values
    const float  fmiss = util::missingValue(fmiss);
    const double dmiss = util::missingValue(dmiss);
    const int    imiss = util::missingValue(imiss);

    if (has(group, variable)) {  // Found the required record in database
      auto var = DataContainer.find(boost::make_tuple(group, variable));
      const std::type_info & typeInput = var->data.get()->type();

      if (typeInput == typeid(float)) {
        oops::Log::debug() << " DataContainer::inquire: inconsistent type : "
                           << " From float to double on "
                           << variable << " @ " << group << std::endl;
        for (std::size_t ii = 0; ii < vsize; ++ii) {
          float zz = boost::any_cast<float>(var->data.get()[ii]);
          if (zz == fmiss) {
            vdata[ii] = dmiss;
          } else {
            vdata[ii] = static_cast<double>(zz);
          }
        }
      } else if (typeInput == typeid(int)) {
          oops::Log::debug() << " DataContainer::inquire: inconsistent type : "
                             << " From int to double on "
                             << variable << " @ " << group << std::endl;
          for (std::size_t ii = 0; ii < vsize; ++ii) {
            int zz = boost::any_cast<int>(var->data.get()[ii]);
            if (zz == imiss) {
              vdata[ii] = dmiss;
            } else {
              vdata[ii] = static_cast<double>(zz);
            }
          }
      } else {  // the in/out types should be the same
        ASSERT(typeInput == typeid(double));
        for (std::size_t ii = 0; ii < vsize; ++ii)
          vdata[ii] = boost::any_cast<double>(var->data.get()[ii]);
      }

    } else {  // Required record is not found
      std::string ErrorMsg =
             "DataContainer::inquire: " + variable + " @ " + group +" is not found";
      ABORT(ErrorMsg);
    }
  }

  void ObsSpaceContainer::inquire(const std::string & group, const std::string & variable,
                                  const std::size_t vsize, float vdata[]) const {
    if (has(group, variable)) {  // Found the required record in database
      auto var = DataContainer.find(boost::make_tuple(group, variable));
      const std::type_info & typeInput = var->data.get()->type();
      ASSERT(typeInput == typeid(float));
      for (std::size_t ii = 0; ii < vsize; ++ii)
        vdata[ii] = boost::any_cast<float>(var->data.get()[ii]);
    } else {  // Required record is not found
      std::string ErrorMsg =
             "DataContainer::inquire: " + variable + " @ " + group +" is not found";
      ABORT(ErrorMsg);
    }
  }

  void ObsSpaceContainer::inquire(const std::string & group, const std::string & variable,
                                  const std::size_t vsize, int vdata[]) const {
    // Obtain the missing values
    const double dmiss = util::missingValue(dmiss);
    const int    imiss = util::missingValue(imiss);

    if (has(group, variable)) {  // Found the required record in database
      auto var = DataContainer.find(boost::make_tuple(group, variable));
      const std::type_info & typeInput = var->data.get()->type();

      if (typeInput == typeid(double)) {
          oops::Log::debug() << " DataContainer::inquire: inconsistent type : "
                             << " From double to int on "
                             << variable << " @ " << group << std::endl;
          for (std::size_t ii = 0; ii < vsize; ++ii) {
            double zz = boost::any_cast<double>(var->data.get()[ii]);
            if (zz == dmiss) {
              vdata[ii] = imiss;
            } else {
              vdata[ii] = static_cast<int>(zz);
            }
          }
      } else {  // the in/out types should be the same
        ASSERT(typeInput == typeid(int));
        for (std::size_t ii = 0; ii < vsize; ++ii)
          vdata[ii] = boost::any_cast<int>(var->data.get()[ii]);
      }
    } else {  // Required record is not found
      std::string ErrorMsg =
             "DataContainer::inquire: " + variable + " @ " + group +" is not found";
      ABORT(ErrorMsg);
    }
  }

  void ObsSpaceContainer::inquire(const std::string & group, const std::string & variable,
                                  const std::size_t vsize, util::DateTime vdata[]) const {
    if (has(group, "date") && has(group, "time")) {  // Found the required record in database
      auto date = DataContainer.find(boost::make_tuple(group, "date"));
      auto time = DataContainer.find(boost::make_tuple(group, "time"));
      const std::type_info & date_type = date->data.get()->type();
      const std::type_info & time_type = time->data.get()->type();
      ASSERT(date_type == typeid(int));
      ASSERT(time_type == typeid(int));
      int vdate, vtime;
      for (std::size_t ii = 0; ii < vsize; ++ii) {
        vdate = boost::any_cast<int>(date->data.get()[ii]);
        vtime = boost::any_cast<int>(time->data.get()[ii]);
        util::DateTime tmp(static_cast<int>(vdate/10000),
                           static_cast<int>((vdate%10000)/100),
                           static_cast<int>((vdate%10000)%100),
                           static_cast<int>(vtime/10000),
                           static_cast<int>((vtime%10000)/100),
                           static_cast<int>((vtime%10000)%100));
        vdata[ii] = tmp;
      }
    } else {  // Required record is not found
      std::string ErrorMsg =
             "DataContainer::inquire: " + variable + " @ " + group +" is not found";
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
