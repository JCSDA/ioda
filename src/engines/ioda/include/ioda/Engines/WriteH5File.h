#pragma once
/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <string>

#include "ioda/Engines/ReaderBase.h"     // For the flen string to vlen string workaround
#include "ioda/Engines/ReaderFactory.h"  // For the flen string to vlen string workaround
#include "ioda/Engines/WriterBase.h"
#include "ioda/Engines/WriterFactory.h"

namespace ioda {
namespace Engines {

//----------------------------------------------------------------------------------------
// WriteH5File
//----------------------------------------------------------------------------------------

// Parameters

class WriteH5FileParameters : public WriterParametersBase {
    OOPS_CONCRETE_PARAMETERS(WriteH5FileParameters, WriterParametersBase)

  public:
    /// Empty for now, but still serves as a concretization of the abstract base class
};

// Classes

class WriteH5File : public WriterBase {
 public:
  typedef WriteH5FileParameters Parameters_;

  // Constructor via parameters
  WriteH5File(const Parameters_ & params, const WriterCreationParameters & createParams);

  void print(std::ostream & os) const override;

 private:
  // parameters
  Parameters_ params_;
};

//----------------------------------------------------------------------------------------
// WriteH5Proc pre-/post-processor
//----------------------------------------------------------------------------------------

class WriteH5Proc : public WriterProcBase {
 public:
  typedef WriteH5FileParameters Parameters_;

  // Constructor via parameters
  WriteH5Proc(const Parameters_ & params, const WriterCreationParameters & createParams);

  /// \brief post processor to be run after the WriteH5File object is destructed
  /// \detail This post processor is associated with WriteH5File, but placed in a
  /// separate class structure so that the hdf5 file can be closed before running this
  /// post processor.
  void post() override;

  void print(std::ostream & os) const override;

 private:
  /// \brief engine parameters
  Parameters_ params_;

  /// \brief generate the names of the files for the post-processor workaround
  /// \param finalFileName is the name for the file that is written by the
  /// post-processor workaround
  /// \param tempFileName is the name for the file (written by the ioda writer) that
  /// is being read by the post-processor workaround
  void workaroundGenFileNames(std::string & finalFileName, std::string & tempFileName);

  /// \brief run the post-processor workaround (change fixed length strings to
  /// variable length strings)
  /// \param finalFileName is the name for the file that is written by the
  /// post-processor workaround
  /// \param tempFileName is the name for the file (written by the ioda writer) that
  /// is being read by the post-processor workaround
  void workaroundFixToVarLenStrings(const std::string & finalFileName,
                                    const std::string & tempFileName);

};

/// \brief parameters for opening the file (written by the idoa writer) for reading
/// in the post-processor
class WorkaroundReaderParameters : public oops::Parameters {
  OOPS_CONCRETE_PARAMETERS(WorkaroundReaderParameters, Parameters)
 public:
  oops::RequiredParameter<Engines::ReaderParametersWrapper> engine{"engine", this};
};

/// \brief parameters for opening the new output file created in the post-processor
class WorkaroundWriterParameters : public oops::Parameters {
  OOPS_CONCRETE_PARAMETERS(WorkaroundWriterParameters, Parameters)
 public:
  oops::RequiredParameter<Engines::WriterParametersWrapper> engine{"engine", this};
};

}  // namespace Engines
}  // namespace ioda
