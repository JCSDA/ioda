#pragma once
/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <string>
#include <vector>

#include "ioda/Engines/ReaderBase.h"

namespace ioda {
namespace Engines {

class Printable;
class ReaderFactory;

//----------------------------------------------------------------------------------------
// Reader factory classes
//----------------------------------------------------------------------------------------

class ReaderFactory {
 public:
  /// \brief Create and return a new instance of an ReaderBase subclass.
  /// \param params Parameters object for the engine creation
  static std::unique_ptr<ReaderBase> create(const ReaderParametersBase & params,
                                            const ReaderCreationParameters & createParams);

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
                                           const ReaderCreationParameters & createParams) = 0;

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
                                   const ReaderCreationParameters & createParams) override {
    const auto &stronglyTypedParameters = dynamic_cast<const Parameters_&>(params);
    return std::make_unique<T>(stronglyTypedParameters, createParams);
  }

  /// \brief Construct a new instance of an ReaderParametersBase subclass.
  std::unique_ptr<ReaderParametersBase> makeParameters() const override {
    return std::make_unique<Parameters_>();
  }
 public:
  explicit ReaderMaker(const std::string & type) : ReaderFactory(type) {}
};

//----------------------------------------------------------------------------------------
// ReaderParameters polymorphic parameters wrapper
//----------------------------------------------------------------------------------------

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
// Reader factory utilities
//----------------------------------------------------------------------------------------

/// \brief create a file reader backend from an eckit configuration
/// \param winStart time window start
/// \param winEnd time window end
/// \param comm MPI communicator for model grouping or io pool
/// \param timeComm MPI communicator for ensemble
/// \param obsVarNames list of variables being assimilated
/// \param isParalleIo if true use any availale parallel IO feature
std::unique_ptr<ReaderBase> constructFileReaderFromConfig(
                const util::DateTime & winStart, const util::DateTime & winEnd,
                const eckit::mpi::Comm & comm, const eckit::mpi::Comm & timeComm,
                const std::vector<std::string> & obsVarNames,
                const bool isParallelIo, const eckit::LocalConfiguration & config);

}  // namespace Engines
}  // namespace ioda

