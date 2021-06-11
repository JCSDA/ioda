/*
 * (C) Crown copyright 2021, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef IO_OBSIOFACTORY_H_
#define IO_OBSIOFACTORY_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <boost/make_unique.hpp>

namespace ioda {

class ObsIo;
class ObsIoParametersBase;
class ObsSpaceParameters;

enum class ObsIoModes {
  READ,
  WRITE
};

class ObsIoFactory {
 public:
  /// \brief Create and return a new instance of an ObsIo subclass.
  ///
  /// If \p mode is set to READ, the type of the instantiated subclass is determined by
  /// the string returned by `parameters.top_level_.obsIoInParameters().type.value()`.
  /// If \p mode is set to WRITE, an ObsIoFileCreate instance is returned unless
  /// `parameters.top_level_.obsOutFile is not set, in which case an exception is thrown.
  static std::shared_ptr<ObsIo> create(ObsIoModes mode,
                                       const ObsSpaceParameters & parameters);

  /// \brief Create and return an instance of the subclass of ObsIoParametersBase
  /// storing parameters of the specified type of ObsIo.
  static std::unique_ptr<ObsIoParametersBase> createParameters(const std::string & name);

  /// \brief Return the names of all ObsIo subclasses that can be created by one of the
  /// registered makers.
  static std::vector<std::string> getMakerNames();

  virtual ~ObsIoFactory() = default;

 protected:
  /// \brief Register a maker able to create instances of the specified ObsIo subclass.
  explicit ObsIoFactory(const std::string &);

 private:
  virtual std::shared_ptr<ObsIo> make(const ObsIoParametersBase & ioParameters,
                                      const ObsSpaceParameters & obsSpaceParameters) = 0;

  virtual std::unique_ptr<ObsIoParametersBase> makeParameters() const = 0;

  static std::map<std::string, ObsIoFactory*> & getMakers();

  static ObsIoFactory& getMaker(const std::string &name);
};

template <class T>
class ObsIoMaker : public ObsIoFactory {
 private:
  typedef typename T::Parameters_ Parameters_;

 public:
  explicit ObsIoMaker(const std::string & name) : ObsIoFactory(name) {}

  std::shared_ptr<ObsIo> make(const ObsIoParametersBase & ioParameters,
                              const ObsSpaceParameters & obsSpaceParameters) override {
    const auto &stronglyTypedIoParameters = dynamic_cast<const Parameters_&>(ioParameters);
    return std::make_shared<T>(stronglyTypedIoParameters, obsSpaceParameters);
  }

  std::unique_ptr<ObsIoParametersBase> makeParameters() const override {
    return boost::make_unique<Parameters_>();
  }
};

}  // namespace ioda

#endif  // IO_OBSIOFACTORY_H_
