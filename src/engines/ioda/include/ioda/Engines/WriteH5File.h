#pragma once
/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include <string>

#include "ioda/Engines/WriterBase.h"

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

}  // namespace Engines
}  // namespace ioda
