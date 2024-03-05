/*
 * (C) Copyright 2022-2023 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/C/ioda_has_variables_c.hpp"

extern "C" {

ioda_has_variables_t ioda_has_variables_c_alloc() {
  return reinterpret_cast<void *>(new ioda::Has_Variables());
}

void ioda_has_variables_c_dtor(ioda_has_variables_t *p) {
  VOID_TO_CXX(ioda::Has_Variables, *p, hvar);
//  delete hvar;
  hvar = nullptr;
}

void ioda_has_variables_c_clone(ioda_has_variables_t *v, ioda_has_variables_t rhs_p) {
  try {
    ioda::Has_Variables **vp = reinterpret_cast<ioda::Has_Variables **>(v);
    VOID_TO_CXX(ioda::Has_Variables, rhs_p, rhs);
    if (rhs == nullptr) {
      *vp = nullptr;
      return;
    }
    /// this is weird but has_variables need to be shallow copy
    *vp = rhs;
    return;
  } catch (std::exception &e) {
    std::cerr << "ioda_has_attributes_c_clone exception " << e.what() << "\n";
    fatal_error();
  }
}

cxx_vector_string_t ioda_has_variables_c_list(ioda_has_variables_t p) {
  try {
    VOID_TO_CXX(ioda::Has_Variables, p, hvar);
    if (hvar == nullptr) {
      std::cerr << "ioda_has_variables_c_list null ptr\n";
      throw std::exception();
    }
    std::vector<std::string> *vs = new std::vector<std::string>(hvar->list());
    return reinterpret_cast<void *>(vs);
  } catch (std::exception &e) {
    std::cerr << "ioda_has_variables_list failed\n";
    fatal_error();
  }
  return nullptr;
}

bool ioda_has_variables_c_exists(void *p, int64_t n, const char *name) {
  try {
    if (n == 0) return false;
    VOID_TO_CXX(ioda::Has_Variables, p, hvar);
    if (hvar == nullptr) {
      std::cerr << "ioda_has_variables_c_exists null ptr\n";
      throw std::exception();
    }
    if (name == nullptr) {
      std::cerr << "ioda_has_variables_c_exists name is null\n";
      throw std::exception();
    }
    std::string name_str(name, n);
    std::cerr << "ioda_has_variables_c_exists name = " << name_str << "\n";
    bool b = hvar->exists(name_str);
    return b;
  } catch (std::exception &e) {
    std::cerr << "ioda_has_variables_exists failed\n";
    fatal_error();
  }
  // put here to make compiler happy
  return false;
}

bool ioda_has_variables_c_remove(void *p, int64_t n, const char *name_str) {
  try {
    VOID_TO_CXX(ioda::Has_Variables, p, hvar);
    if (hvar == nullptr) 
    {
      std::cerr << "ioda_has_variables_c_remove null ptr\n";
      throw std::exception();
    }
    if (name_str == nullptr) 
    {
      std::cerr << "ioda_has_variables_c_remove name is null\n";
      throw std::exception();
    }
    hvar->remove(std::string(name_str, n));
    return true;
  } 
  catch (std::exception &e) 
  {
    std::cerr << "ioda_variables_c_remove failed\n";
    std::cerr << e.what() << "\n";
    fatal_error();
  }
  return false;
}

ioda_variable_t ioda_has_variables_c_open(void *p, int64_t n, const char *name) {
  try {
    VOID_TO_CXX(ioda::Has_Variables, p, hvar);
    if (hvar == nullptr) 
    {
      std::cerr << "ioda_has_variables_c_open null ptr\n";
      throw std::exception();
    }
    if (name == nullptr) 
    {
      std::cerr << "ioda_has_variables_c_open name is null\n";
      throw std::exception();
    }
    ioda::Variable *var = new ioda::Variable(hvar->open(std::string(name, n)));
    return reinterpret_cast<void *>(var);
  } 
  catch (std::exception &e) 
  {
    std::cerr << "ioda_has_variables_open failed\n";
    fatal_error();
  }
  return nullptr;
}

#define IODA_FUN(NAME, TYPE)                                                                       \
  ioda_variable_t ioda_has_variables_c_create##NAME(void *p, int64_t name_sz, const char *name,    \
                                                    int64_t ndims, int64_t *dims) {                \
    try {                                                                                          \
      VOID_TO_CXX(ioda::Has_Variables, p, has_var);                                                \
      if (has_var == nullptr) {                                                                    \
        std::cerr << "ioda_has_Variables_create has_variables is null\n";                          \
        throw std::exception();                                                                    \
      }                                                                                            \
      if (name == nullptr) {                                                                       \
        std::cerr << "ioda_has_Variables_create name is null\n";                                   \
        throw std::exception();                                                                    \
      }                                                                                            \
      std::vector<ioda::Dimensions_t> vdims(dims, dims + ndims);                                   \
      std::string names(name, name_sz);                                                            \
      ioda::Variable *vout = new ioda::Variable(has_var->create<TYPE>(names, vdims, vdims));       \
      if (!vout->isA<TYPE>()) {                                                                    \
        std::cerr << "ioda_has_variables_c_create the created variable does not exist\n";          \
      } else {                                                                                     \
        std::cerr << "ioda_has_variables_c_create the create variable is not the right type ?\n";  \
      }                                                                                            \
      if (has_var->exists(names) && vout->isA<TYPE>()) {                                           \
        return vout;                                                                               \
      }                                                                                            \
      std::cerr << "the created var does not exist?\n";                                            \
      throw std::exception();                                                                      \
    } catch (std::exception & e) {                                                                 \
      std::cerr << "ioda_has_variables_create exception ";                                         \
      std::cerr << e.what() << "\n";                                                               \
      fatal_error();                                                                               \
    }                                                                                              \
    return 0x0;                                                                                    \
  }

IODA_FUN(_float, float)
IODA_FUN(_double, double)
IODA_FUN(_ldouble, long double)
IODA_FUN(_char, char)
IODA_FUN(_int16, int16_t)
IODA_FUN(_int32, int32_t)
IODA_FUN(_int64, int64_t)
IODA_FUN(_uint16, uint16_t)
IODA_FUN(_uint32, uint32_t)
IODA_FUN(_uint64, uint64_t)
IODA_FUN(_str, std::vector<std::string>)
#undef IODA_FUN

#define IODA_FUN(NAME, TYPE)                                                                       \
  ioda_variable_t ioda_has_variables_c_create2##NAME(                                              \
    void *p, int64_t name_sz, const char *name, int64_t ndims, int64_t *dims, int64_t *max_dims,   \
    ioda_variable_creation_parameters_t creation_p) {                                              \
    try {                                                                                          \
      VOID_TO_CXX(ioda::Has_Variables, p, has_var);                                                \
      if (has_var == nullptr) {                                                                    \
        std::cerr << "ioda_has_Variables_create2 has_variables is null\n";                         \
        throw std::exception();                                                                    \
      }                                                                                            \
      if (name == nullptr) {                                                                       \
        std::cerr << "ioda_has_Variables_create2 name is null\n";                                  \
        throw std::exception();                                                                    \
      }                                                                                            \
      std::vector<ioda::Dimensions_t> dvec(dims, dims + ndims);                                    \
      std::vector<ioda::Dimensions_t> max_dvec(max_dims, max_dims + ndims);                        \
      if (dims == nullptr || max_dims == nullptr) {                                                \
        std::cerr << "ioda_has_Variables_create2 dims is null\n";                                  \
        throw std::exception();                                                                    \
      }                                                                                            \
      VOID_TO_CXX(ioda::VariableCreationParameters, creation_p, cparams);                          \
      if (cparams == nullptr) {                                                                    \
        std::cerr << "ioda_has_Variables_create2 creation parameters is null\n";                   \
        throw std::exception();                                                                    \
      }                                                                                            \
      std::string nstr(name, name_sz);                                                             \
      std::cerr << "check 1\n";                                                                    \
      ioda::Variable *var                                                                          \
        = new ioda::Variable(has_var->create<TYPE>(nstr, dvec, max_dvec, *cparams));               \
      return reinterpret_cast<void *>(var);                                                        \
    } catch (std::exception & e) {                                                                 \
      std::cerr << "ioda_has_variables_create2 exception ";                                        \
      std::cerr << e.what() << "\n";                                                               \
    }                                                                                              \
    return 0x0;                                                                                    \
  }

IODA_FUN(_float, float)
IODA_FUN(_double, double)
IODA_FUN(_ldouble, long double)
IODA_FUN(_char, char)
IODA_FUN(_int16, int16_t)
IODA_FUN(_int32, int32_t)
IODA_FUN(_int64, int64_t)
IODA_FUN(_uint16, uint16_t)
IODA_FUN(_uint32, uint32_t)
IODA_FUN(_uint64, uint64_t)
IODA_FUN(_str, std::vector<std::string>)
#undef IODA_FUN

}
