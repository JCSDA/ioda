/*
 * (C) Copyright 2020-2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_string
 * @{
 * \file String_c.cpp
 * \brief @link ioda_string C bindings @endlink.
 */

#include "ioda/C/String_c.h"
#include "./structs_c.h"

namespace ioda {
namespace C {
namespace Strings {

void clear(ioda_string *this_)
{
  C_TRY;
  if (!this_)        throw Exception("this_ must not be null", ioda_Here());
  if (!this_->data_) throw Exception("this_->data_ must not be null", ioda_Here());
  this_->data_->str.clear();
  C_CATCH_AND_TERMINATE;
}

void destruct(ioda_string *this_)
{
  if (!this_) return;
  delete this_->data_;
  delete this_;
}

size_t get(const ioda_string *this_, char *outstr, size_t outstr_len)
{
  C_TRY;
  if (!this_)        throw Exception("this_ must not be null", ioda_Here());
  if (!this_->data_) throw Exception("this_->data_ must not be null", ioda_Here());
  if (!outstr)       throw Exception("outstr must not be null", ioda_Here());
  if (!outstr_len)   throw Exception("outstr_len must be nonzero", ioda_Here());

  // Always clearing out a string buffer in case the calling language
  // ignores null characters (like Fortran).
  std::memset(outstr, 0, outstr_len);

  size_t sz_tocopy = std::min(this_->data_->str.size(), outstr_len);
  std::copy_n(this_->data_->str.c_str(), sz_tocopy, outstr);
  // By this point, outstr is not yet null-terminated.
  // Figure out where to put the terminating NULL byte.
  if (sz_tocopy == outstr_len)
  {
    outstr[outstr_len - 1] = '\0';
    return outstr_len; // Could not copy entire string.
  }
  else
  {
    outstr[sz_tocopy] = '\0';
    return sz_tocopy;
  }

  C_CATCH_AND_TERMINATE;
}

size_t set(struct ioda_string *this_, const char *instr, size_t instr_len)
{
  C_TRY;
  if (!this_)        throw Exception("this_ must not be null", ioda_Here());
  if (!this_->data_) throw Exception("this_->data_ must not be null", ioda_Here());
  if (!instr)        throw Exception("instr must not be null", ioda_Here());
  this_->data_->str = std::string(instr, instr_len);
  return instr_len;
  C_CATCH_AND_TERMINATE;
}

size_t size(const ioda_string *this_)
{
  C_TRY;
  if (!this_)        throw Exception("this_ must not be null", ioda_Here());
  if (!this_->data_) throw Exception("this_->data_ must not be null", ioda_Here());
  return this_->data_->str.size();
  C_CATCH_AND_TERMINATE;
}

ioda_string *construct(); // Defined one function down. Circular resolution.

ioda_string *constructFromCstr(const char* buf)
{
  C_TRY;
  if (!buf) throw Exception("buf must not be null", ioda_Here());
  auto ret = construct();
  ret->data_->str = std::string{buf};
  return ret;
  C_CATCH_AND_TERMINATE;
}

ioda_string *copy(const ioda_string* from)
{
  C_TRY;
  if (!from) throw Exception("from must not be null", ioda_Here());
  auto ret = construct();
  ret->data_->str = from->data_->str;
  return ret;
  C_CATCH_AND_TERMINATE;
}

IODA_DL ioda_string *construct()
{
  C_TRY;
  auto ret = new ioda_string{
    &construct, &constructFromCstr, &destruct, &clear, &get,
    &size, &set, &size, &copy,
    new c_ioda_string};
  return ret;
  C_CATCH_AND_TERMINATE;
}

// This variable is re-declared as extern in ioda_c.cpp.
ioda_string general_c_ioda_string {
  &construct, &constructFromCstr, &destruct, &clear, &get,
    &size, &set, &size, &copy, nullptr
};


}  // end namespace Strings
}  // end namespace C
}  // end namespace ioda


extern "C" {

IODA_DL void ioda_string_ret_t_destruct(ioda_string_ret_t* obj) {
  C_TRY;
  if (!obj) throw ioda::Exception("Parameter 'obj' cannot be null.", ioda_Here());
  if (!obj->strings) throw ioda::Exception("Parameter 'obj'->strings cannot be null.", ioda_Here());
  for (size_t i = 0; i < obj->n; ++i) {
    if (obj->strings[i] != nullptr) {
      delete[] obj->strings[i];
    }
  }
  delete[] obj->strings;
  delete obj;
  C_CATCH_AND_TERMINATE;
}
}

/// @}
