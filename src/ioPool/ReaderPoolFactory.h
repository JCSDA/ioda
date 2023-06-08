#pragma once
/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/ioPool/IoPoolParameters.h"
#include "ioda/ioPool/ReaderPoolBase.h"

namespace ioda {

//----------------------------------------------------------------------------------------
// ReaderPool factory classes
//----------------------------------------------------------------------------------------

class ReaderPoolFactory {
 public:
  /// \brief Create and return a new instance of an ReaderPoolBase subclass.
  /// \param configParams Parameters object for the pool creation
  static std::unique_ptr<ReaderPoolBase> create(const IoPoolParameters & configParams,
                                            const ReaderPoolCreationParameters & createParams);

  /// \brief Return the names of all ReaderPoolBase subclasses that can be created by
  /// one of the registered makers.
  static std::vector<std::string> getMakerNames();

  virtual ~ReaderPoolFactory() = default;

 protected:
  /// \brief Register a maker able to create instances of the specified ReaderPoolBase subclass.
  explicit ReaderPoolFactory(const std::string & name);

 private:
  /// \brief Construct a new instance of an ReaderPoolBase subclass.
  /// \param configParams Parameters object for the pool construction
  virtual std::unique_ptr<ReaderPoolBase> make(const IoPoolParameters & configParams,
                                    const ReaderPoolCreationParameters & createParams) = 0;

  /// \brief Return reference to the map holding the registered makers
  static std::map<std::string, ReaderPoolFactory*> & getMakers();

  /// \brief Return reference to a registered maker
  static ReaderPoolFactory& getMaker(const std::string & name);
};

template <class T>
class ReaderPoolMaker : public ReaderPoolFactory {
  /// \brief Construct a new instance of an ReaderPoolBase subclass.
  /// \param configParams Parameters object for the pool construction
  std::unique_ptr<ReaderPoolBase> make(const IoPoolParameters & configParams,
                            const ReaderPoolCreationParameters & createParams) override {
    return std::make_unique<T>(configParams, createParams);
  }

 public:
  explicit ReaderPoolMaker(const std::string & name) : ReaderPoolFactory(name) {}
};

}  // namespace ioda

