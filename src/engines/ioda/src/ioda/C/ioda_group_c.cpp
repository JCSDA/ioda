/*
 * (C) Copyright 2022-2023 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/C/ioda_group_c.hpp"

extern "C" {

void * ioda_group_c_alloc()
{
    try {
        return reinterpret_cast<void*>(new ioda::Group());
    } catch (std::exception& e) {
        std::cerr << "ioda_group_c_alloc failed\n";
        fatal_error();    
    }
    return nullptr;
}

void ioda_group_c_dtor(void **p) {
    if ( p == nullptr) return;
    void *p_ = *p;
    VOID_TO_CXX(ioda::Group,p_,g);
    if ( g == nullptr) return;
    delete g;
    g = nullptr;
    *p = nullptr;
}

void ioda_group_c_clone(void **t_p,void *rhs_p)
{
    try {
        ioda::Group ** t = 
            reinterpret_cast< ioda::Group ** >(t_p);
        VOID_TO_CXX(ioda::Group,rhs_p,rhs);
        if ( rhs == nullptr) {
            return;
        }
        *t = new ioda::Group(*rhs);
        t_p = reinterpret_cast< void ** >(t);
        return;
    } catch ( std::exception& e) {
        std::cerr << "ioda_group_c_clone exception " << e.what() << "\n";
        fatal_error();  
    }
}

void * ioda_group_c_list(void *p) {
    try {
        VOID_TO_CXX(ioda::Group,p,g);
        if ( g == nullptr ) {
            std::cerr << "ioda_group_c_list null pointer\n";
            throw std::exception();
        }
        std::vector<std::string> * vs = new std::vector<std::string>(g->list());
        return reinterpret_cast<void*>(vs);
    } catch (std::exception& e) {
        std::cerr << "ioda_group_c_list failed " << e.what() << "\n";
        fatal_error();
    }
    return nullptr;
}

int ioda_group_c_exists(void *p,int64_t sz,const void *name_p) {
    try {
        VOID_TO_CXX(ioda::Group,p,g);
        VOID_TO_CXX(const char,name_p,name);
        if ( g == nullptr) {
            std::cerr << "ioda_group_c_exists group null pointer in arguments\n";
            throw std::exception();
        }
        if ( name == nullptr) {
            std::cerr << "ioda_group_c_exists char null pointer in arguments\n";
            throw std::exception();
        }
        std::string name_str(name);
        if ( g->exists(name_str) )   
        {
                return 1;
        }else{
                return 0;
        }
    } catch (std::exception& e) {
        std::cerr << "ioda_group_c_exists failed " << e.what() << "\n";
        fatal_error();
    }
    return -1;
}

void * ioda_group_c_create(void *p,int64_t sz,const void *name_p) {
    try {
        VOID_TO_CXX(ioda::Group,p,g);
        VOID_TO_CXX(const char,name_p,name);
        if ( g == nullptr || name == nullptr) {
            std::cerr << "ioda_group_c_create null pointer\n";
            throw std::exception();
        }
        ioda::Group * r = new ioda::Group( g->create(std::string(name)) ); 
        return reinterpret_cast<void*>(r);
    } catch (std::exception& e) {
        std::cerr << "ioda_group_c_create failed " << e.what() << "\n";
        fatal_error();
    }
    return nullptr;
}

void * ioda_group_c_open(void *p,int64_t sz,const void *name_p) {
    try {
        VOID_TO_CXX(ioda::Group,p,g);
        VOID_TO_CXX(const char,name_p,name);
        if ( g == nullptr || name == nullptr) {
            std::cerr << "ioda_group_c_open null pointer\n";
            throw std::exception();
        }
        ioda::Group * r =  new ioda::Group(g->open(std::string(name))); 
        return reinterpret_cast<void*>(r);
    } catch (std::exception& e) {
        std::cerr << "ioda_group_c_open failed " << e.what() << "\n";
        fatal_error();
    }
    return nullptr;
}

void * ioda_group_c_has_attributes(void *g_p)
{
    try {
        VOID_TO_CXX(ioda::Group,g_p,g);
        if ( g == nullptr ) {
            std::cerr << "ioda_group_c_has_attributes null pointer\n";
            throw std::exception();
        }
        ioda::Has_Attributes * has_a =  &(g->atts); 
        return reinterpret_cast<void*>(has_a);
    } catch (std::exception& e) {
        std::cerr << "ioda_group_c_has_attributes failed " << e.what() << "\n";
        fatal_error();
    }
    return nullptr;
}

void * ioda_group_c_has_variables(void *g_p)
{
     try {
        VOID_TO_CXX(ioda::Group,g_p,g);
        if ( g == nullptr ) {
            std::cerr << "ioda_group_c_has_variables null pointer\n";
            throw std::exception();
        }
        ioda::Has_Variables * has_v =  &(g->vars); 
        return reinterpret_cast<void*>(has_v);
    } catch (std::exception& e) {
        std::cerr << "ioda_group_c_has_variables failed " << e.what() << "\n";
        fatal_error();
    }
    return nullptr;
}

}
