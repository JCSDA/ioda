/*
 * (C) Copyright 2022-2023 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "ioda/C/cxx_vector_int.hpp"
extern "C" {
cxx_vector_int_t cxx_vector_int_c_alloc()
{
    void * p = CXX_TO_VOID(new std::vector<int>());
    return p;
}

void cxx_vector_int_c_dealloc(cxx_vector_int_t  *p)
{
    if ( p == nullptr) return;
    VOID_TO_CXX(std::vector<int>,(*p),v);
    if ( v != nullptr) delete v;
}

void cxx_vector_int_c_copy(cxx_vector_int_t * p, cxx_vector_int_t q)
{
    try {
        if (p == nullptr) throw std::invalid_argument(" first ptr argument is null");
        VOID_TO_CXX(std::vector<int>,q,qv);
        if ( qv == nullptr) throw std::invalid_argument(" second ptr argument is null");
        VOID_TO_CXX(std::vector<int>,(*p),pv);
        if ( pv == nullptr) {
            pv = new std::vector<int>(*qv);
        }else{
            *pv = *qv;
        }
        p = reinterpret_cast< void ** >(pv); 
    } catch (std::exception& e) {
        std::cerr << __FUNCTION__ << " exception " << e.what() << "\n";
        fatal_error();
    }
}

void cxx_vector_int_c_push_back(cxx_vector_int_t vp,int x)
{
    try {
        VOID_TO_CXX(std::vector<int>,vp,v);
        if (v == nullptr) throw std::invalid_argument("ptr argument is null");
        v->push_back(x);
    } catch (std::exception& e) {
        std::cerr << __FUNCTION__ << " exception " << e.what() << "\n";
        fatal_error();
    }
}

void cxx_vector_int_c_set(cxx_vector_int_t vp,int64_t i,int x)
{
    try {
        VOID_TO_CXX(std::vector<int>,vp,v);
        if (v == nullptr) throw std::invalid_argument("ptr argument is null");
        v->at(i) = x;
    } catch (std::exception& e) {
        std::cerr << __FUNCTION__ << " exception " << e.what() << "\n";
        fatal_error();
    }
}

int cxx_vector_int_c_get(cxx_vector_int_t vp,int64_t i)
{
    try {
        VOID_TO_CXX(std::vector<int>,vp,v);
        if (v == nullptr) throw std::invalid_argument("ptr argument is null");
        int x = v->at(i);
        return x;
    } catch (std::exception& e) {
        std::cerr << __FUNCTION__ << " exception " << e.what() << "\n";
        fatal_error();
    }
    return -1;
}

int64_t cxx_vector_int_c_size(cxx_vector_int_t vp)
{
    try {
        VOID_TO_CXX(std::vector<int>,vp,v);
        if (v == nullptr) throw std::invalid_argument("ptr argument is null");
        int64_t n = static_cast<int64_t>(v->size());
        return n;
    } catch (std::exception& e) {
        std::cerr << __FUNCTION__ << " exception " << e.what() << "\n";
        fatal_error();
    }
    return 0L;
}

void cxx_vector_int_c_resize(cxx_vector_int_t vp,int64_t n)
{
    try {
        VOID_TO_CXX(std::vector<int>,vp,v);
        if (v == nullptr) throw std::invalid_argument("ptr argument is null");
        v->resize(n);
    } catch (std::exception& e) {
        std::cerr << __FUNCTION__ << " exception " << e.what() << "\n";
        fatal_error();
    }
}

void cxx_vector_int_c_clear(cxx_vector_int_t vp)
{
    try {
        VOID_TO_CXX(std::vector<int>,vp,v);
        if (v == nullptr) throw std::invalid_argument("ptr argument is null");
        v->clear();
    } catch (std::exception& e) {
        std::cerr << __FUNCTION__ << " exception " << e.what() << "\n";
        fatal_error();
    }
}

int cxx_vector_int_c_empty(cxx_vector_int_t vp)
{
    try {
        VOID_TO_CXX(std::vector<int>,vp,v);
        if (v == nullptr) throw std::invalid_argument("ptr argument is null");
        if ( v->empty()) return 1;
    } catch (std::exception& e) {
        std::cerr << __FUNCTION__ << " exception " << e.what() << "\n";
        fatal_error();
    }
    return 0;
}

}