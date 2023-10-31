/*
 * (C) Copyright 2022-2023 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "ioda/C/cxx_string.hpp"
extern "C" {
cxx_string_t cxx_string_c_alloc()
{
    try {
        std::string * s = new std::string();
        return CXX_TO_VOID(s);
    } catch (std::exception& e) {
        std::cerr << __FUNCTION__ << " exception " << e.what();
        fatal_error();
    }
    return nullptr;
}

void cxx_string_c_dealloc(cxx_string_t * s)
{
    if ( s == nullptr ) return;
    VOID_TO_CXX(std::string,(*s),str);
    if (str == nullptr) return;
    delete str;
    str = nullptr;
}

void cxx_string_c_set(cxx_string_t *s,const char *val)
{
    try {
        if ( s == nullptr) {
            // need to assign a value for s, string is not allocated
            std::string * str = new std::string(val);
            s = reinterpret_cast< void** >(&str);
        } else {
            // stringis allocated
            VOID_TO_CXX(std::string,(*s),str);
            if ( str == nullptr ) {
                str = new std::string(val);
            }else{
                *str = val;
            }
            s = reinterpret_cast<void**>(&str);
        }
    } catch (std::exception& e) {
        std::cerr << __FUNCTION__ << " exception " << e.what() << "\n";
        fatal_error();
    }
}

char * cxx_string_c_get(cxx_string_t s)
{
    try {
        if ( s == nullptr ) {
            throw std::invalid_argument(" string ptr is null");
        }
        VOID_TO_CXX(std::string,s,str);
        char * r = Strdup(str->c_str());
        return r;
    } catch (std::exception& e) {
        std::cerr << __FUNCTION__ << " exception " << e.what() << "\n";
        fatal_error();
    }
    return nullptr;
}

void cxx_string_c_copy(cxx_string_t *s,cxx_string_t o)
{
    try {
        if ( o == nullptr ) {
            throw std::invalid_argument(" 2nd string ptr is null");
        }
        VOID_TO_CXX(std::string,o,ostr);
        if ( s == nullptr ) {
            std::string * str = new std::string(*ostr);
            s = reinterpret_cast< void** >(&str);
            return;
        }
        VOID_TO_CXX(std::string,(*s),str);
        if ( str == nullptr) {
            str = new std::string(*ostr);
            s = reinterpret_cast< void** >(&str);
            return;
        }else{
            *str = *ostr;
        }
    } catch (std::exception& e) {
        std::cerr << __FUNCTION__ << " exception " << e.what() << "\n";
        fatal_error();
    }
}

int64_t cxx_string_c_size(cxx_string_t s)
{
    try {
        VOID_TO_CXX(std::string,s,str);
        if ( s == nullptr ) {
            std::cerr << __FUNCTION__ << " warning size called on null ptr\n";
            return 0;
        }
        size_t sz = str->size();
        return static_cast< int64_t >(sz);
    } catch (std::exception& e) {
        std::cerr << __FUNCTION__ << " exception " << e.what() << "\n";
        fatal_error();
    }
    return -1;
}

void cxx_string_c_clear(cxx_string_t s)
{
    try {
        if ( s == nullptr ) {
            throw std::invalid_argument(" string ptr is null");
        }
        VOID_TO_CXX(std::string,s,str);
        str->clear();
    } catch (std::exception& e) {
        std::cerr << __FUNCTION__ << " exception " << e.what() << "\n";
        fatal_error();
    }
}

void cxx_string_c_append_str(cxx_string_t l,cxx_string_t r)
{
    try {
        VOID_TO_CXX(std::string,l,lstr);
        VOID_TO_CXX(std::string,r,rstr);
        if ( lstr == nullptr ) throw std::invalid_argument(" left str is null ptr");
        if ( rstr == nullptr ) throw std::invalid_argument(" right str is null ptr");
        *lstr += *rstr;
    } catch (std::exception& e) {
        std::cerr << __FUNCTION__ << " exception " << e.what() << "\n";
        fatal_error();
    }
}

void cxx_string_c_append(cxx_string_t l,const char * r)
{
    try {
        VOID_TO_CXX(std::string,l,lstr);
        if ( l == nullptr ) throw std::invalid_argument(" left str is null ptr");
        *lstr += r;
    } catch (std::exception& e) {
        std::cerr << __FUNCTION__ << " exception " << e.what() << "\n";
        fatal_error();
    }
}
}  // end extern "C"
