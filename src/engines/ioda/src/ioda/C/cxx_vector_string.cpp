/*
 * (C) Copyright 2022-2023 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "ioda/C/cxx_vector_string.hpp"
extern "C" {

cxx_vector_string_t  cxx_vector_string_c_alloc() {
    return CXX_TO_VOID(new std::vector<std::string>());
}

void cxx_vector_string_c_dealloc(cxx_vector_string_t *p)
{
    try {
        if ( p == nullptr ) return;
        VOID_TO_CXX(std::vector<std::string>,(*p),str);
        if ( str == nullptr ) return;
        delete str;
        p = nullptr;
    } catch (std::exception& e) {
        std::cerr << __FUNCTION__ << " exception " << e.what() << "\n";
        fatal_error();
    }
}

int64_t cxx_vector_string_c_element_size(cxx_vector_string_t p,int64_t i)
{
    try {
        VOID_TO_CXX(std::vector<std::string>,p,vstr);
        if ( vstr == nullptr ) throw std::invalid_argument("vector string pointer is null");
        const std::string& si = vstr->at(i);
        return static_cast<int64_t>(si.size());
    } catch (std::exception& e) {
        std::cerr << __FUNCTION__ << " exception " << e.what() << "\n";
        fatal_error();
    }
    return -1;
}

int64_t cxx_vector_string_c_size(cxx_vector_string_t p)
{
    try {
        VOID_TO_CXX(std::vector<std::string>,p,vstr);
        if ( vstr == nullptr ) throw std::invalid_argument("vector string pointer is null");
        return static_cast<int64_t>( vstr->size() );
    } catch (std::exception& e) {
        std::cerr << __FUNCTION__ << " exception " << e.what() << "\n";
        fatal_error();
    }
    return -1;
}

cxx_string_t cxx_vector_string_c_get_str(cxx_vector_string_t p,int64_t i)
{
    try {
        VOID_TO_CXX(std::vector<std::string>,p,vstr);
        if ( vstr == nullptr ) throw std::invalid_argument("vector string pointer is null");
        if ( i < 0) throw std::invalid_argument(" i < 0 for get_str ");
        std::string * s = new std::string( vstr->at(i) );
        return CXX_TO_VOID(s);
    } catch (std::exception& e) {
        std::cerr << __FUNCTION__ << " exception " << e.what() << "\n";
        fatal_error();
    }
    return nullptr;
}

char * cxx_vector_string_c_get(cxx_vector_string_t p,int64_t i)
{
    try {
        VOID_TO_CXX(std::vector<std::string>,p,vstr);
        if ( vstr == nullptr ) throw std::invalid_argument("vector string pointer is null");
        if ( i < 0) throw std::invalid_argument(" i < 0 for get ");
        const std::string& s = vstr->at(i);
        return Strdup(s.c_str());
    } catch (std::exception& e) {
        std::cerr << __FUNCTION__ << " exception " << e.what() << "\n";
        fatal_error();
    }
    return nullptr;
}

void cxx_vector_string_c_set_str(cxx_vector_string_t p,int64_t i,cxx_string_t s)
{
    try {
        VOID_TO_CXX(std::vector<std::string>,p,vstr);
        if ( vstr == nullptr ) throw std::invalid_argument("vector string pointer is null");
        VOID_TO_CXX(std::string,s,str);
        if ( str == nullptr) throw std::invalid_argument("string pointer is null");
        if ( i < 0) throw std::invalid_argument(" i < 0 for set_str ");
        std::cerr << "set str " << vstr->size() << " " << i<< "\n";
        vstr->at(i) = *str;
    } catch (std::exception& e) {
        std::cerr << __FUNCTION__ << " exception " << e.what() << "\n";
        fatal_error();
    }
}

void cxx_vector_string_c_set(cxx_vector_string_t p,int64_t i,const char * str)
{
    try {
        VOID_TO_CXX(std::vector<std::string>,p,vstr);
        if ( vstr == nullptr ) throw std::invalid_argument("vector string pointer is null");
        if ( i < 0) throw std::invalid_argument(" i < 0 for set ");
        std::cerr << "set " << vstr->size() << " " << i <<"\n";
        if ( str != nullptr ) {
            vstr->at(i) = std::string(str);
        }else{
            vstr->at(i) = std::string();
        }
    } catch (std::exception& e) {
        std::cerr << __FUNCTION__ << " exception " << e.what() << "\n";
        fatal_error();
    }
}

void cxx_vector_string_c_push_back(cxx_vector_string_t vs,const char *v)
{
    try {
        std::string rstr(v);
        VOID_TO_CXX(std::vector<std::string>,vs,vstr);
        if ( vstr == nullptr ) throw std::invalid_argument(" vector pointer is null ");
        vstr->push_back(std::string(v));
    } catch (std::exception& e) {
        std::cerr << __FUNCTION__ << " exception " << e.what() << "\n";
        fatal_error();
    }
}

void cxx_vector_string_c_push_back_str(cxx_vector_string_t v,cxx_string_t r)
{
    try {
        VOID_TO_CXX(std::string,r,rstr);
        if ( rstr == nullptr) throw std::invalid_argument(" string pointer is null ");
        VOID_TO_CXX(std::vector<std::string>,v,vstr);
        if ( vstr == nullptr ) throw std::invalid_argument(" vector pointer is null ");
        vstr->push_back(*rstr);
    } catch (std::exception& e) {
        std::cerr << __FUNCTION__ << " exception " << e.what() << "\n";
        fatal_error();
    }
}

void cxx_vector_string_c_copy(cxx_vector_string_t * v,cxx_vector_string_t o)
{
    try {
        VOID_TO_CXX(std::vector<std::string>,o,ostr);
        if ( ostr == nullptr) throw std::invalid_argument("vector string pointer rhs is null");
        if ( v == nullptr ) {
            std::vector<std::string> * v1 = new std::vector<std::string>(*ostr);
            v = reinterpret_cast< void ** >((&v1));
            return;
        }
        VOID_TO_CXX(std::vector<std::string>,(*v),vstr);
        //  do we need this?
        if ( vstr == nullptr ) {
             vstr = new std::vector<std::string>();
             v = reinterpret_cast<void**>(&vstr);
        }
        (*vstr) = (*ostr);
    } catch (std::exception& e) {
        std::cerr << __FUNCTION__ << " exception " << e.what() << "\n";
        fatal_error();
    }
}


int cxx_vector_string_c_empty(cxx_vector_string_t v)
{
    try {
        VOID_TO_CXX(std::vector<std::string>,v,vstr);
        //  do we need this?
        if ( vstr == nullptr ) return 1;
        return (vstr->empty()) ? 1:0;
    } catch (std::exception& e) {
        std::cerr << __FUNCTION__ << " exception " << e.what() << "\n";
        fatal_error();
    }
    return -1;
}

void cxx_vector_string_c_resize(cxx_vector_string_t v, int64_t n)
{
    try {
        VOID_TO_CXX(std::vector<std::string>,v,vstr);
        if ( vstr == nullptr ) throw std::invalid_argument(" null ptr for argument ");
        if ( n < 0) throw std::invalid_argument(" n < 0 for resize ");

        std::cerr << " c++ resize " << n << "\n";
        vstr->resize(static_cast<size_t>(n));
        std::cerr << " c++ resize after " << vstr->size() << "\n";
        
    } catch (std::exception& e) {
        std::cerr << __FUNCTION__ << " exception " << e.what() << "\n";
        fatal_error();
    }
}

void cxx_vector_string_c_clear(cxx_vector_string_t v)
{
    try {
        VOID_TO_CXX(std::vector<std::string>,v,vstr);
        if ( vstr == nullptr ) throw std::invalid_argument(" null ptr for argument ");
        vstr->clear();
    } catch (std::exception& e) {
        std::cerr << __FUNCTION__ << " exception " << e.what() << "\n";
        fatal_error();
    }
}

}  // end extern "C"