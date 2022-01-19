/*
 * (C) Copyright 2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! @file Params.cpp
* @brief Parameters for ioda file validation.
*/

#include "./Params.h"

namespace ioda_validate {

const char SeverityParameterTraitsHelper::enumTypeName[] = "Severity";
const util::NamedEnumerator<Severity> SeverityParameterTraitsHelper::namedValues[]
  = {{Severity::Trace, "Trace"},
     {Severity::Debug, "Debug"},
     {Severity::Info, "Info"},
     {Severity::Warn, "Warn"},
     {Severity::Error, "Error"}};

const char TypeParameterTraitsHelper::enumTypeName[] = "Type";
const util::NamedEnumerator<Type> TypeParameterTraitsHelper::namedValues[]
  = {{Type::Unspecified, "Unspecified"},
     {Type::SameAsVariable, "SameAsVariable"},
     {Type::Float, "Float"},
     {Type::Double, "Double"},
     {Type::Int8, "Int8"},
     {Type::Int16, "Int16"},
     {Type::Int32, "Int32"},
     {Type::Int64, "Int64"},
     {Type::UInt8, "UInt8"},
     {Type::UInt16, "UInt16"},
     {Type::UInt32, "UInt32"},
     {Type::UInt64, "UInt64"},
     {Type::StringVLen, "StringVLen"},
     {Type::StringFixedLen, "StringFixedLen"},
     {Type::Datetime, "Datetime"},
     {Type::Char, "Char"},
     {Type::SChar, "SChar"},
     {Type::Enum, "Enum"}};
}  // end namespace ioda_validate
