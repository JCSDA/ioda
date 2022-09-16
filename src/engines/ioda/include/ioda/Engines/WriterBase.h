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
class WriterFactory;

//----------------------------------------------------------------------------------------
// Writer parameters base class
//----------------------------------------------------------------------------------------

/// \brief Parameters base class for the parameters subclasses associated with the
/// WriterBase subclasses.
class WriterParametersBase : public oops::Parameters {
    OOPS_ABSTRACT_PARAMETERS(WriterParametersBase, Parameters)

 public:
    /// \brief Type of the WriterBase subclass to use.
    oops::RequiredParameter<std::string> type{"type", this};
};

/// \brief Polymorphic parameter holding an instance of a subclass of WriterParametersBase.
class WriterParametersWrapper : public oops::Parameters {
  OOPS_CONCRETE_PARAMETERS(WriterParametersWrapper, Parameters)
 public:
  /// After deserialization, holds an instance of a subclass of WriterParametersBase
  /// controlling the behavior of a ioda backend engine. The type of the subclass is
  /// determined by the value of the "type" key in the Configuration object from
  /// which this object is deserialized.
  oops::RequiredPolymorphicParameter<WriterParametersBase, WriterFactory>
      engineParameters{"type", "type of the IODA backend engine", this};
};

//----------------------------------------------------------------------------------------
// Writer base class
//----------------------------------------------------------------------------------------

/// \details The WriterBase class along with its subclasses are responsible for providing
/// a ioda engines Group object that is backed up by a particular ioda engines backend
/// for the purpose of writing out obs data.
///
/// \author Stephen Herbener (JCSDA)

class WriterBase : public util::Printable {
 public:
    WriterBase(const util::DateTime & winStart, const util::DateTime & winEnd,
               const eckit::mpi::Comm & comm, const eckit::mpi::Comm & timeComm,
               const std::vector<std::string> & obsVarNames);
    virtual ~WriterBase() {}

    /// \brief return the backend that stores the data
    inline ioda::ObsGroup getObsGroup() { return obs_group_; }

    /// \brief return the backend that stores the data
    inline const ioda::ObsGroup getObsGroup() const { return obs_group_; }

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
};

//----------------------------------------------------------------------------------------
// Writer factory classes
//----------------------------------------------------------------------------------------

class WriterFactory {
 public:
  /// \brief Create and return a new instance of an WriterBase subclass.
  /// \param params Parameters object for the engine creation
  static std::unique_ptr<WriterBase> create(const WriterParametersBase & params,
                                            const util::DateTime & winStart,
                                            const util::DateTime & winEnd,
                                            const eckit::mpi::Comm & comm,
                                            const eckit::mpi::Comm & timeComm,
                                            const std::vector<std::string> & obsVarNames);

  /// \brief Create and return an instance of the subclass of ObsOperatorParametersBase
  /// storing parameters of observation operators of the specified type.
  static std::unique_ptr<WriterParametersBase> createParameters(const std::string & type);

  /// \brief Return the names of all WriterBase subclasses that can be created by one of the
  /// registered makers.
  static std::vector<std::string> getMakerNames();

  virtual ~WriterFactory() = default;

 protected:
  /// \brief Register a maker able to create instances of the specified WriterBase subclass.
  explicit WriterFactory(const std::string & type);

 private:
  /// \brief Construct a new instance of an WriterBase subclass.
  /// \param params Parameters object for the engine construction
  virtual std::unique_ptr<WriterBase> make(const WriterParametersBase & params,
                                           const util::DateTime & winStart,
                                           const util::DateTime & winEnd,
                                           const eckit::mpi::Comm & comm,
                                           const eckit::mpi::Comm & timeComm,
                                           const std::vector<std::string> & obsVarNames) = 0;

  /// \brief Construct a new instance of an WriterParametersBase subclass.
  virtual std::unique_ptr<WriterParametersBase> makeParameters() const = 0;

  /// \brief Return reference to the map holding the registered makers
  static std::map<std::string, WriterFactory*> & getMakers();

  /// \brief Return reference to a registered maker
  static WriterFactory& getMaker(const std::string & type);
};

template <class T>
class WriterMaker : public WriterFactory {
  typedef typename T::Parameters_ Parameters_;

  /// \brief Construct a new instance of an WriterBase subclass.
  /// \param params Parameters object for the engine construction
  std::unique_ptr<WriterBase> make(const WriterParametersBase & params,
                                   const util::DateTime & winStart,
                                   const util::DateTime & winEnd,
                                   const eckit::mpi::Comm & comm,
                                   const eckit::mpi::Comm & timeComm,
                                   const std::vector<std::string> & obsVarNames) override {
    const auto &stronglyTypedParameters = dynamic_cast<const Parameters_&>(params);
    return std::make_unique<T>(stronglyTypedParameters, winStart, winEnd,
                               comm, timeComm, obsVarNames);
  }

  /// \brief Construct a new instance of an WriterParametersBase subclass.
  std::unique_ptr<WriterParametersBase> makeParameters() const override {
    return std::make_unique<Parameters_>();
  }
 public:
  explicit WriterMaker(const std::string & type) : WriterFactory(type) {}
};


}  // namespace Engines
}  // namespace ioda

