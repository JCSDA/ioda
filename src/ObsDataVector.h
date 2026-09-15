/*
 * (C) Copyright 2018-2026 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef OBSDATAVECTOR_H_
#define OBSDATAVECTOR_H_

#include <ostream>
#include <string>
#include <vector>

#include "oops/base/ObsVariables.h"
#include "oops/util/ObjectCounter.h"
#include "oops/util/Printable.h"

#include "ioda/ObsSpaceAssociated.h"

namespace ioda {

class ObsSpace;
class ObsVector;

//-----------------------------------------------------------------------------

template <typename DATATYPE> using ObsDataRow = std::vector<DATATYPE>;

//-----------------------------------------------------------------------------
//! ObsDataVector<DATATYPE> handles vectors of data of type DATATYPE in observation space

template<typename DATATYPE>
class ObsDataVector: public ObsSpaceAssociated,
                     public util::Printable,
                     private util::ObjectCounter<ObsDataVector<DATATYPE> > {
 public:
  static const std::string classname() {return "ioda::ObsDataVector";}

  ObsDataVector(ObsSpace &, const oops::ObsVariables &,
                const std::string & grp = "", const bool fail = true,
                const bool skipDerived = false);
  ObsDataVector(ObsSpace &, const std::string &,
                const std::string & grp = "", const bool fail = true,
                const bool skipDerived = false);
  explicit ObsDataVector(ObsVector &);
  ObsDataVector(const ObsDataVector &);
  ObsDataVector(ObsDataVector &&);
  ~ObsDataVector();

  ObsDataVector & operator= (const ObsDataVector &);
  ObsDataVector & operator= (ObsDataVector &&);

  void zero();
  void mask(const ObsDataVector<int> &);

  // Read ObsDataVector.
  void read(const std::string &, const bool fail = true,
            const bool skipDerived = false);
  // Read appended ObsData.
  void readAppended(const std::string &, const bool fail = true,
            const bool skipDerived = false);
  void save(const std::string &) const;

/// \brief   Assign to all variables of this ObsDataVector, the values in the ObsVector vect
/// \details Loop through all variables in the ObsDataVector, matching them up with variables
///          in ObsVector vect - if present in vect, copy across values into the matching
///          variables of ObsDataVector, taking care to convert missing values. Error if an
///          ObsDataVector variable is not found in vect.
/// \param[in]  vect  ObsVector whose values are to be assigned to this ObsDataVector.
  void assignToExistingVariables(const ObsVector & vect);

// Methods below are used by UFO but not by OOPS
  const ObsSpace & space() const {return obsdb_;}
  size_t nvars() const {return nvars_;}  // Size in (local) memory
  size_t nlocs() const {return nlocs_;}  // Size in (local) memory
  bool has(const std::string & vargrp) const {return obsvars_.has(vargrp);}

  const ObsDataRow<DATATYPE> & operator[](const size_t ii) const {return rows_.at(ii);}
  ObsDataRow<DATATYPE> & operator[](const size_t ii) {return rows_.at(ii);}

  const ObsDataRow<DATATYPE> & operator[](const std::string var) const
    {return rows_.at(obsvars_.find(var));}
  ObsDataRow<DATATYPE> & operator[](const std::string var) {return rows_.at(obsvars_.find(var));}

  const std::string & obstype() const;
  const oops::ObsVariables & varnames() const {return obsvars_;}

  void reduce(const std::vector<bool> & keepLocs) override;
  void append() override;
  void syncAppend() override {indexAppend_ = nlocs_;}
  void zeroAppended();

 private:
  void print(std::ostream &) const override;
  // Carry out reading of the ObsDataVector.
  void doRead(const std::string &, const bool fail = true,
            const bool skipDerived = false, const std::size_t & stloc = 0);

  ObsSpace & obsdb_;
  oops::ObsVariables obsvars_;
  size_t nvars_;
  size_t nlocs_;
  std::size_t indexAppend_;
  std::vector<ObsDataRow<DATATYPE> > rows_;
  const DATATYPE missing_;
};

}  // namespace ioda

#endif  // OBSDATAVECTOR_H_
