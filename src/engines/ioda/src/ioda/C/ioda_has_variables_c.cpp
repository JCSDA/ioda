/*
 * (C) Copyright 2022-2023 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/C/ioda_has_variables_c.hpp"

extern "C" {

void * ioda_has_variables_c_alloc()
{
     return reinterpret_cast<void*>(new ioda::Has_Variables());
}


void ioda_has_variables_c_dtor(void **p)
{
    VOID_TO_CXX(ioda::Has_Variables,*p,hvar);
    if (hvar) {
        delete hvar;
    }
    *p = nullptr;
}

void ioda_has_variables_c_clone(void **t_p,void *rhs_p)
{
    try {
        ioda::Has_Variables ** t = 
            reinterpret_cast< ioda::Has_Variables ** >(t_p);
        VOID_TO_CXX(ioda::Has_Variables,rhs_p,rhs);
        if ( *t != nullptr) {
            delete *t;    
        }
        if ( rhs == nullptr) {
            return;
        }
        *t = new ioda::Has_Variables(*rhs);
        t_p = reinterpret_cast< void ** >(t);
        return;
    } catch ( std::exception& e) {
        std::cerr << "ioda_has_variables_c_clone exception " << e.what() << "\n";
        fatal_error();  
    }
}

void * ioda_has_variables_c_list(void *p)
{
    try {
        VOID_TO_CXX(ioda::Has_Variables,p,hvar);
        if ( hvar == nullptr ) {
            std::cerr << "ioda_has_variables_c_list null ptr\n";
            throw std::exception();
        }
        return reinterpret_cast<void*>(new std::vector<std::string>(hvar->list()));
    } catch (std::exception& e) {
        std::cerr << "ioda_has_variables_list failed\n";
        fatal_error();        
    }
    return nullptr;
}

bool ioda_has_variables_c_exists(void *p,int64_t n,const void *name_str)
{
    try {
        VOID_TO_CXX(ioda::Has_Variables,p,hvar);
        VOID_TO_CXX(const char,name_str,name);
        if ( hvar == nullptr) {
            std::cerr << "ioda_has_variables_c_exists null ptr\n";
            throw std::exception();
        }
        if ( name_str == nullptr) {
            std::cerr << "ioda_has_variables_c_exists name is null\n";
            throw std::exception();
        }
        bool b = hvar->exists(std::string(name,n));
        return b;
    } catch (std::exception& e) {
        std::cerr << "ioda_has_variables_exists failed\n";
        fatal_error();        
    }
    // put here to make compiler happy
    return false;
}

bool ioda_has_variables_c_remove(void *p,int64_t n,const void *name_str)
{
    try {
        VOID_TO_CXX(ioda::Has_Variables,p,hvar);
        VOID_TO_CXX(const char,name_str,name);
        if ( hvar == nullptr) {
            std::cerr << "ioda_has_variables_c_remove null ptr\n";
            throw std::exception();
        }
        if ( name_str == nullptr) {
            std::cerr << "ioda_has_variables_c_remove name is null\n";
            throw std::exception();
        }
        hvar->remove(std::string(name,n));
        return true;
    } catch (ioda::Exception& e) {
        return false;    
    } catch (std::exception& e) {
        std::cerr << "ioda_variables_c_remove failed\n";
        std::cerr << e.what() << "\n";
        fatal_error();  
    }
    return false;
}

void * ioda_has_variables_c_open(void *p,int64_t n,const void *name_str)
{
    try {
        VOID_TO_CXX(ioda::Has_Variables,p,hvar);
        VOID_TO_CXX(const char,name_str,name);
        if ( hvar == nullptr) {
            std::cerr << "ioda_has_variables_c_open null ptr\n";
            throw std::exception();
        }
        if ( name_str == nullptr) {
            std::cerr << "ioda_has_variables_c_open name is null\n";
            throw std::exception();
        }
        ioda::Variable * var = new ioda::Variable(hvar->open(std::string(name,n)));
        return reinterpret_cast<void*>(var);
    } catch (std::exception& e) {
        std::cerr << "ioda_has_variables_open failed\n";
        fatal_error();        
    }
    return nullptr;
}

#define IODA_FUN(NAME,TYPE)\
void *  ioda_has_variables_c_create##NAME(void *p,int64_t name_sz,			\
    const void *name_p, int64_t ndims, int64_t *dims_) {				\
    try { 										\
         VOID_TO_CXX(ioda::Has_Variables,p,has_var);					\
         if (has_var == nullptr ) {							\
             std::cerr << "ioda_has_Variables_create has_variables is null\n";		\
             throw std::exception();							\
         }										\
         VOID_TO_CXX(const char,name_p,name);						\
         if ( name == nullptr ) {							\
             std::cerr << "ioda_has_Variables_create name is null\n";			\
             throw std::exception();         						\
         } 										\
         if ( dims_ == nullptr ) { 							\
             std::cerr << "ioda_has_Variables_create dims is null\n";			\
             throw std::exception();         						\
         }										\
         ioda::Variable * var = new ioda::Variable(					\
           has_var->create< TYPE >( std::string(name,name_sz) ,				\
                 std::vector<ioda::Dimensions_t>(dims_,dims_+ndims) ) );		\
         return reinterpret_cast<void*>(var);;						\
    } catch (std::exception& e) {							\
         std::cerr << "ioda_has_variables_create exception "; 				\
         std::cerr << e.what() << "\n"; 						\
    }											\
    return 0x0;										\
}  

IODA_FUN(_float,float)
IODA_FUN(_double,double)
IODA_FUN(_ldouble,long double)
IODA_FUN(_char,char)
IODA_FUN(_int16,int16_t)
IODA_FUN(_int32,int32_t)
IODA_FUN(_int64,int64_t)
IODA_FUN(_uint16,uint16_t)
IODA_FUN(_uint32,uint32_t)
IODA_FUN(_uint64,uint64_t)
IODA_FUN(_str,std::vector<std::string>)
#undef IODA_FUN

#define IODA_FUN(NAME,TYPE)\
void *  ioda_has_variables_c_create2##NAME(void *p,int64_t name_sz,			\
    const void *name_p, int64_t ndims, int64_t *dims_,					\
    int64_t *max_dims_,void *creation_p) {						\
    try { 										\
         VOID_TO_CXX(ioda::Has_Variables,p,has_var);					\
         if (has_var == nullptr ) {							\
             std::cerr << "ioda_has_Variables_create has_variables is null\n";		\
             throw std::exception();							\
         }										\
         VOID_TO_CXX(const char,name_p,name);						\
         if ( name == nullptr ) {							\
             std::cerr << "ioda_has_Variables_create name is null\n";			\
             throw std::exception();         						\
         } 										\
         if ( dims_ == nullptr || max_dims_ == nullptr ) { 				\
             std::cerr << "ioda_has_Variables_create dims is null\n";			\
             throw std::exception();         						\
         }										\
         VOID_TO_CXX(ioda::VariableCreationParameters,creation_p,cparams);		\
         if ( cparams == nullptr) {							\
             std::cerr << "ioda_has_Variables_create creation parameters is null\n";	\
             throw std::exception();         						\
         }										\
         std::string nstr(name,name_sz);						\
         std::vector<ioda::Dimensions_t> dvec(dims_,dims_+ndims);			\
         std::vector<ioda::Dimensions_t> mdvec(max_dims_,max_dims_+ndims);		\
         ioda::Variable * var = new ioda::Variable(					\
                has_var->create< TYPE >(nstr,dvec,mdvec,*cparams) );			\
         return reinterpret_cast<void*>(var);;						\
    } catch (std::exception& e) {							\
         std::cerr << "ioda_has_variables_create exception "; 				\
         std::cerr << e.what() << "\n"; 						\
    }											\
    return 0x0;										\
}  

IODA_FUN(_float,float)
IODA_FUN(_double,double)
IODA_FUN(_ldouble,long double)
IODA_FUN(_char,char)
IODA_FUN(_int16,int16_t)
IODA_FUN(_int32,int32_t)
IODA_FUN(_int64,int64_t)
IODA_FUN(_uint16,uint16_t)
IODA_FUN(_uint32,uint32_t)
IODA_FUN(_uint64,uint64_t)
IODA_FUN(_str,std::vector<std::string>)
#undef IODA_FUN

}
