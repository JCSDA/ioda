/*
 * (C) Copyright 2022 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_vecstring
 * @{
 * \file VecString_c.cpp
 * \brief @link ioda_vecstring C bindings @endlink. Needed for reads.
 */

#include "ioda/C/VecString_c.h"
#include "./structs_c.h"

namespace ioda {
namespace C {
namespace VecStrings {

void clear(ioda_VecString *this_)
{
  C_TRY;
  if (!this_)        throw Exception("this_ must not be null", ioda_Here());
  if (!this_->data_) throw Exception("this_->data_ must not be null", ioda_Here());
  this_->data_->vec.clear();
  C_CATCH_AND_TERMINATE;
}

void destruct(ioda_VecString *this_)
{
  if (!this_) return;
  delete this_->data_;
  delete this_;
}

size_t getAsCharArray(const ioda_VecString *this_, size_t n, char *outstr, size_t outstr_len)
{
  C_TRY;
  if (!this_)        throw Exception("this_ must not be null", ioda_Here());
  if (!this_->data_) throw Exception("this_->data_ must not be null", ioda_Here());
  if (!outstr)       throw Exception("outstr must not be null", ioda_Here());
  if (!outstr_len)   throw Exception("outstr_len must be nonzero", ioda_Here());
  if (this_->data_->vec.size() <= n)
    throw Exception("Out-of-bounds access on element", ioda_Here()).add("n", n).add("size", this_->data_->vec.size());

  size_t sz_tocopy = std::min(this_->data_->vec.at(n).size(), outstr_len);
  std::copy_n(this_->data_->vec.at(n).c_str(), sz_tocopy, outstr);
  
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

size_t getAsCharArray2(const ioda_VecString *this_, size_t n, char *outstr, size_t outstr_len, char empty_char)
{
  C_TRY;
  if (!this_)        throw Exception("this_ must not be null", ioda_Here());
  if (!this_->data_) throw Exception("this_->data_ must not be null", ioda_Here());
  if (!outstr)       throw Exception("outstr must not be null", ioda_Here());
  if (!outstr_len)   throw Exception("outstr_len must be nonzero", ioda_Here());
  if (this_->data_->vec.size() <= n)
    throw Exception("Out-of-bounds access on element", ioda_Here()).add("n", n).add("size", this_->data_->vec.size());

  std::memset(outstr, empty_char, outstr_len);

  size_t sz_tocopy = std::min(this_->data_->vec.at(n).size(), outstr_len);
  std::copy_n(this_->data_->vec.at(n).c_str(), sz_tocopy, outstr);
  
  return outstr_len;
  C_CATCH_AND_TERMINATE;
}

size_t setFromCharArray(struct ioda_VecString *this_, size_t n, const char *instr, size_t instr_len)
{
  C_TRY;
  if (!this_)        throw Exception("this_ must not be null", ioda_Here());
  if (!this_->data_) throw Exception("this_->data_ must not be null", ioda_Here());
  if (!instr)        throw Exception("instr must not be null", ioda_Here());
  if (this_->data_->vec.size() <= n)
    throw Exception("Out-of-bounds access on element", ioda_Here()).add("n", n).add("size", this_->data_->vec.size());
  this_->data_->vec.at(n) = std::string(instr, instr_len);
  return instr_len;
  C_CATCH_AND_TERMINATE;
}

size_t elementSize(const ioda_VecString *this_, size_t n)
{
  C_TRY;
  if (!this_)        throw Exception("this_ must not be null", ioda_Here());
  if (!this_->data_) throw Exception("this_->data_ must not be null", ioda_Here());
  if (this_->data_->vec.size() <= n)
    throw Exception("Out-of-bounds access on element", ioda_Here()).add("n", n).add("size", this_->data_->vec.size());
  return this_->data_->vec.at(n).size();
  C_CATCH_AND_TERMINATE;
}

size_t size(const ioda_VecString *this_)
{
  C_TRY;
  if (!this_)        throw Exception("this_ must not be null", ioda_Here());
  if (!this_->data_) throw Exception("this_->data_ must not be null", ioda_Here());
  return this_->data_->vec.size();
  C_CATCH_AND_TERMINATE;
}

void resize(ioda_VecString *this_, size_t n)
{
  C_TRY;
  if (!this_)        throw Exception("this_ must not be null", ioda_Here());
  if (!this_->data_) throw Exception("this_->data_ must not be null", ioda_Here());
  this_->data_->vec.resize(n);
  C_CATCH_AND_TERMINATE;
}

ioda_VecString *construct(); // Defined later. Circular resolution.

ioda_VecString *copy(const ioda_VecString* from)
{
  C_TRY;
  if (!from) throw Exception("from must not be null", ioda_Here());
  auto ret = construct();
  ret->data_->vec = from->data_->vec;
  return ret;
  C_CATCH_AND_TERMINATE;
}

ioda_VecString *construct()
{
  C_TRY;
  auto ret = new ioda_VecString{
    &destruct, &construct, &copy, &clear,
    &getAsCharArray, &getAsCharArray2, &setFromCharArray, &elementSize, &size, &resize,
    new c_ioda_VecString};
  return ret;
  C_CATCH_AND_TERMINATE;
}

// This variable is re-declared as extern in ioda_c.cpp.
ioda_VecString general_c_ioda_vecstring {
  &destruct, &construct, &copy, &clear,
    &getAsCharArray, &getAsCharArray2, &setFromCharArray,
    &elementSize, &size, &resize, nullptr
};

ioda_VecString* vecToVecString(const std::vector<std::string>& src) {
  C_TRY;
  auto ret = construct();
  if (!ret) throw Exception("construct failed.", ioda_Here());
  ret->data_->vec = src;
  return ret;
  C_CATCH_AND_TERMINATE;
}

}  // end namespace VecStrings
}  // end namespace C
}  // end namespace ioda

/// @}
