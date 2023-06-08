#pragma once
/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <string>
#include <vector>

#include "ioda/ioPool/IoPoolParameters.h"
#include "ioda/ioPool/WriterPoolBase.h"

namespace ioda {

//----------------------------------------------------------------------------------------
// WriterPool factory classes
//----------------------------------------------------------------------------------------

class WriterPoolFactory {
 public:
  /// \brief Create and return a new instance of an WriterPoolBase subclass.
  /// \param configParams Parameters object for the pool creation
  static std::unique_ptr<WriterPoolBase> create(const IoPoolParameters & configParams,
                                            const WriterPoolCreationParameters & createParams);

  /// \brief Return the names of all WriterPoolBase subclasses that can be created by one of the
  /// registered makers.
  static std::vector<std::string> getMakerNames();

  virtual ~WriterPoolFactory() = default;

 protected:
  /// \brief Register a maker able to create instances of the specified WriterPoolBase subclass.
  explicit WriterPoolFactory(const std::string & name);

 private:
  /// \brief Construct a new instance of an WriterPoolBase subclass.
  /// \param configParams Parameters object for the pool construction
  virtual std::unique_ptr<WriterPoolBase> make(const IoPoolParameters & configParams,
                                    const WriterPoolCreationParameters & createParams) = 0;

  /// \brief Return reference to the map holding the registered makers
  static std::map<std::string, WriterPoolFactory*> & getMakers();

  /// \brief Return reference to a registered maker
  static WriterPoolFactory& getMaker(const std::string & name);
};

template <class T>
class WriterPoolMaker : public WriterPoolFactory {
  /// \brief Construct a new instance of an WriterPoolBase subclass.
  /// \param configParams Parameters object for the pool construction
  std::unique_ptr<WriterPoolBase> make(const IoPoolParameters & configParams,
                            const WriterPoolCreationParameters & createParams) override {
    return std::make_unique<T>(configParams, createParams);
  }

 public:
  explicit WriterPoolMaker(const std::string & name) : WriterPoolFactory(name) {}
};

}  // namespace ioda

