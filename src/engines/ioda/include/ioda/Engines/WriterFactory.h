#pragma once
/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <string>
#include <vector>

#include "ioda/Engines/WriterBase.h"

namespace ioda {
namespace Engines {

//----------------------------------------------------------------------------------------
// Writer factory classes
//----------------------------------------------------------------------------------------

class WriterFactory {
 public:
  /// \brief Create and return a new instance of an WriterBase subclass.
  /// \param params Parameters object for the engine creation
  static std::unique_ptr<WriterBase> create(const WriterParametersBase & params,
                                            const WriterCreationParameters & createParams);

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
                                           const WriterCreationParameters & createParams) = 0;

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
                                   const WriterCreationParameters & createParams) override {
    const auto &stronglyTypedParameters = dynamic_cast<const Parameters_&>(params);
    return std::make_unique<T>(stronglyTypedParameters, createParams);
  }

  /// \brief Construct a new instance of an WriterParametersBase subclass.
  std::unique_ptr<WriterParametersBase> makeParameters() const override {
    return std::make_unique<Parameters_>();
  }
 public:
  explicit WriterMaker(const std::string & type) : WriterFactory(type) {}
};

//----------------------------------------------------------------------------------------
// WriterParameters polymorphic parameters wrapper
//----------------------------------------------------------------------------------------
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
// Writer pre-/post-processor factory classes
//----------------------------------------------------------------------------------------

class WriterProcFactory {
 public:
  /// \brief Create and return a new instance of an WriterBase subclass.
  /// \param params Parameters object for the engine creation
  static std::unique_ptr<WriterProcBase> create(const WriterParametersBase & params,
                                                const WriterCreationParameters & createParams);

  /// \brief Return the names of all WriterBase subclasses that can be created by one of the
  /// registered makers.
  static std::vector<std::string> getMakerNames();

  virtual ~WriterProcFactory() = default;

 protected:
  /// \brief Register a maker able to create instances of the specified WriterBase subclass.
  explicit WriterProcFactory(const std::string & type);

 private:
  /// \brief Construct a new instance of an WriterBase subclass.
  /// \param params Parameters object for the engine construction
  virtual std::unique_ptr<WriterProcBase> make(const WriterParametersBase & params,
                                               const WriterCreationParameters & createParams) = 0;

  /// \brief Return reference to the map holding the registered makers
  static std::map<std::string, WriterProcFactory*> & getMakers();

  /// \brief Return reference to a registered maker
  static WriterProcFactory& getMaker(const std::string & type);
};

template <class T>
class WriterProcMaker : public WriterProcFactory {
  typedef typename T::Parameters_ Parameters_;

  /// \brief Construct a new instance of an WriterProcBase subclass.
  /// \param params Parameters object for the engine construction
  std::unique_ptr<WriterProcBase> make(const WriterParametersBase & params,
                                       const WriterCreationParameters & createParams) override {
    const auto &stronglyTypedParameters = dynamic_cast<const Parameters_&>(params);
    return std::make_unique<T>(stronglyTypedParameters, createParams);
  }

 public:
  explicit WriterProcMaker(const std::string & type) : WriterProcFactory(type) {}
};

//----------------------------------------------------------------------------------------
// Writer factory utilities
//----------------------------------------------------------------------------------------

/// \brief create a file writer backend from an eckit configuration
/// \param comm MPI communicator for model grouping or io pool
/// \param timeComm MPI communicator for ensemble
/// \param createMultipleFiles if true create one file per task
/// \param isParalleIo if true use any availale parallel IO feature
std::unique_ptr<WriterBase> constructFileWriterFromConfig(
                const eckit::mpi::Comm & comm, const eckit::mpi::Comm & timeComm,
                const bool createMultipleFiles, const bool isParallelIo,
                const eckit::LocalConfiguration & config);

}  // namespace Engines
}  // namespace ioda

