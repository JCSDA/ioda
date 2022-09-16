#pragma once
/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <string>
#include <vector>

#include "eckit/mpi/Comm.h"

#include "oops/util/parameters/OptionalParameter.h"
#include "oops/util/parameters/Parameter.h"
#include "oops/util/parameters/Parameters.h"
#include "oops/util/parameters/RequiredParameter.h"
#include "oops/util/parameters/RequiredPolymorphicParameter.h"
#include "oops/util/Printable.h"

#include "ioda/Engines/EngineUtils.h"
#include "ioda/ObsGroup.h"

namespace ioda {
namespace Engines {

class Printable;
class ReaderFactory;

//----------------------------------------------------------------------------------------
// ReaderFactory parameters base class
//----------------------------------------------------------------------------------------

/// \brief Parameters base class for the parameters subclasses associated with the
/// ReaderBase subclasses.
class ReaderParametersBase : public oops::Parameters {
    OOPS_ABSTRACT_PARAMETERS(ReaderParametersBase, Parameters)

 public:
    /// \brief Type of the ReaderBase subclass to use.
    oops::RequiredParameter<std::string> type{"type", this};
};

/// \brief Polymorphic parameter holding an instance of a subclass of ReaderParametersBase.
class ReaderParametersWrapper : public oops::Parameters {
  OOPS_CONCRETE_PARAMETERS(ReaderParametersWrapper, Parameters)
 public:
  /// After deserialization, holds an instance of a subclass of ReaderParametersBase
  /// controlling the behavior of a ioda backend engine. The type of the subclass is
  /// determined by the value of the "type" key in the Configuration object from
  /// which this object is deserialized.
  oops::RequiredPolymorphicParameter<ReaderParametersBase, ReaderFactory>
      engineParameters{"type", "type of the IODA backend engine", this};
};

//----------------------------------------------------------------------------------------
// Reader base class
//----------------------------------------------------------------------------------------

/// \details The ReaderBase class along with its subclasses are responsible for providing
/// a ioda engines Group object that is backed up by a particular ioda engines backend for
/// the purpose of reading in obs data.
///
/// \author Stephen Herbener (JCSDA)

class ReaderBase : public util::Printable {
 public:
    ReaderBase(const util::DateTime & winStart, const util::DateTime & winEnd,
               const eckit::mpi::Comm & comm, const eckit::mpi::Comm & timeComm,
               const std::vector<std::string> & obsVarNames);
    virtual ~ReaderBase() {}

    /// \brief return the backend that stores the data
    inline ioda::ObsGroup getObsGroup() { return obs_group_; }

    /// \brief return the backend that stores the data
    inline const ioda::ObsGroup getObsGroup() const { return obs_group_; }

    /// \brief return the input file name (empty string when using a generator)
    std::string fileName() { return fileName_; }

    /// \brief return true if the locations data (lat, lon, datetime) need to be checked
    /// \details The locations check filters out locations that:
    ///    1. have a datetime value that falls outside the DA timing window
    ///    2. have missing values in either of lat and lon values
    /// Typically you want to enable the check for file backends, and disable the check
    /// for generator backends. The default is true (enabled).
    virtual bool applyLocationsCheck() const { return true; }

 protected:
    //------------------ protected functions ----------------------------------
    /// \brief print() for oops::Printable base class
    /// \param ostream output stream
    virtual void print(std::ostream & os) const = 0;

    //------------------ protected variables ----------------------------------
    /// \brief ObsGroup container associated with the selected backend engine
    ioda::ObsGroup obs_group_;

    /// \brief DA window start
    const util::DateTime winStart_;

    /// \brief DA window end
    const util::DateTime winEnd_;

    /// \brief primary MPI communicator
    const eckit::mpi::Comm & comm_;

    /// \brief time bin MPI communicator
    const eckit::mpi::Comm & timeComm_;

    /// \brief list of varible names that will be used downstream
    const std::vector<std::string> obsVarNames_;

    /// \brief input file name for those readers that use a file
    std::string fileName_;
};

//----------------------------------------------------------------------------------------
// Reader factory classes
//----------------------------------------------------------------------------------------

class ReaderFactory {
 public:
  /// \brief Create and return a new instance of an ReaderBase subclass.
  /// \param params Parameters object for the engine creation
  static std::unique_ptr<ReaderBase> create(const ReaderParametersBase & params,
                                            const util::DateTime & winStart,
                                            const util::DateTime & winEnd,
                                            const eckit::mpi::Comm & comm,
                                            const eckit::mpi::Comm & timeComm,
                                            const std::vector<std::string> & obsVarNames);

  /// \brief Create and return an instance of the subclass of ObsOperatorParametersBase
  /// storing parameters of observation operators of the specified type.
  static std::unique_ptr<ReaderParametersBase> createParameters(const std::string & type);

  /// \brief Return the names of all ReaderBase subclasses that can be created by
  /// one of the registered makers.
  static std::vector<std::string> getMakerNames();

  virtual ~ReaderFactory() = default;

 protected:
  /// \brief Register a maker able to create instances of the specified ReaderBase subclass.
  explicit ReaderFactory(const std::string & type);

 private:
  /// \brief Construct a new instance of an ReaderBase subclass.
  /// \param params Parameters object for the engine construction
  virtual std::unique_ptr<ReaderBase> make(const ReaderParametersBase & params,
                                           const util::DateTime & winStart,
                                           const util::DateTime & winEnd,
                                           const eckit::mpi::Comm & comm,
                                           const eckit::mpi::Comm & timeComm,
                                           const std::vector<std::string> & obsVarNames) = 0;

  /// \brief Construct a new instance of an ReaderParametersBase subclass.
  virtual std::unique_ptr<ReaderParametersBase> makeParameters() const = 0;

  /// \brief Return reference to the map holding the registered makers
  static std::map<std::string, ReaderFactory*> & getMakers();

  /// \brief Return reference to a registered maker
  static ReaderFactory& getMaker(const std::string & type);
};

template <class T>
class ReaderMaker : public ReaderFactory {
  typedef typename T::Parameters_ Parameters_;

  /// \brief Construct a new instance of an ReaderBase subclass.
  /// \param params Parameters object for the engine construction
  std::unique_ptr<ReaderBase> make(const ReaderParametersBase & params,
                                   const util::DateTime & winStart,
                                   const util::DateTime & winEnd,
                                   const eckit::mpi::Comm & comm,
                                   const eckit::mpi::Comm & timeComm,
                                   const std::vector<std::string> & obsVarNames) override {
    const auto &stronglyTypedParameters = dynamic_cast<const Parameters_&>(params);
    return std::make_unique<T>(stronglyTypedParameters, winStart, winEnd,
                               comm, timeComm, obsVarNames);
  }

  /// \brief Construct a new instance of an ReaderParametersBase subclass.
  std::unique_ptr<ReaderParametersBase> makeParameters() const override {
    return std::make_unique<Parameters_>();
  }
 public:
  explicit ReaderMaker(const std::string & type) : ReaderFactory(type) {}
};


}  // namespace Engines
}  // namespace ioda

