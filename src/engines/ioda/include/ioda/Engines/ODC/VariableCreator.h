#pragma once
/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */


#include <memory>
#include <string>
#include "RowsByLocation.h"
#include "VariableReaderBase.h"

namespace ioda {

struct VariableCreationParameters;

namespace Engines {

class ContainerFacade;

namespace ODC {

class DataFromSQL;
class VariableReaderBase;

/// \brief Creates an ioda variable and fills it with values extracted from a column of a data table
/// loaded from an ODB file.
class VariableCreator {
public:
  /// \brief Constructor.
  ///
  /// \param name
  ///   Name of the variable to be created.
  /// \param column
  ///   Name of the ODB column from which variable values will be extracted.
  /// \param member
  ///   Name of a member of a bitfield ODB column from which variable values will be extracted.
  ///   Should be empty for non-bitfield columns.
  /// \param hasChannelAxis
  ///   True if the variable will have a Channel dimension (in addition to the Location dimension),
  ///   false otherwise.
  /// \param readerParameters
  ///   Configuration of an object that will be responsible for the extraction of variable values
  ///   at individual locations.
  VariableCreator(const std::string &name, const std::string &column, const std::string &member,
                  bool hasChannelAxis, const VariableReaderParametersBase &readerParameters);

  /// \brief Creates an ioda variable and fills it with values extracted from `sqlData`.
  ///
  /// \param og
  ///   ObsGroup that will receive the newly created variable.
  /// \param rowsByLocation
  ///   An object identifying ODB rows associated with individual locations.
  /// \param sqlData
  ///   Data table loaded from an ODB file, containing the column specified in the constructor and
  ///   any other columns required by the variable reader whose configuration was passed  the
  ///   constructor.
  void createVariable(ContainerFacade &container,
                      const RowsByLocation &rowsByLocation,
                      const DataFromSQL &sqlData) const;

private:
  template <typename T>
  void createTypedVariable(ContainerFacade &container,
                           const RowsByLocation &rowsByLocation, size_t numValuesPerLocation,
                           const VariableReaderBase& reader) const;

  std::string name_;
  std::string column_;
  std::string member_;
  std::unique_ptr<VariableReaderParametersBase> readerParameters_;
  bool hasChannelAxis_;
};

}  // namespace ODC
}  // namespace Engines
}  // namespace ioda
