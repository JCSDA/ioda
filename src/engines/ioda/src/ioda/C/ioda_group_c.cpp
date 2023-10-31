/*
 * (C) Copyright 2022-2023 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/C/ioda_group_c.hpp"

extern "C" {

ioda_group_t ioda_group_c_alloc()
{
    try {
        return reinterpret_cast<void*>(new ioda::Group());
    } catch (std::exception& e) {
        std::cerr << "ioda_group_c_alloc failed\n";
        fatal_error();    
    }
    return nullptr;
}

void ioda_group_c_dtor(ioda_group_t *p) {
    if ( p == nullptr) return;
    VOID_TO_CXX(ioda::Group,*p,g);
    if ( g == nullptr) return;
    delete g;
    g = nullptr;
    *p = nullptr;
}

void ioda_group_c_clone(ioda_group_t *t_p,ioda_group_t rhs_p)
{
    try {
        ioda::Group ** t = 
            reinterpret_cast< ioda::Group ** >(t_p);
        VOID_TO_CXX(ioda::Group,rhs_p,rhs);
        if ( rhs == nullptr) {
            return;
        }
        *t = new ioda::Group(*rhs);
        t_p = reinterpret_cast< ioda_group_t * >(t);
        return;
    } catch ( std::exception& e) {
        std::cerr << "ioda_group_c_clone exception " << e.what() << "\n";
        fatal_error();  
    }
}

cxx_vector_string_t ioda_group_c_list(ioda_group_t p) {
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

int ioda_group_c_exists(ioda_group_t p,int64_t sz,const char *name) {
    try {
        VOID_TO_CXX(ioda::Group,p,g);
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

ioda_group_t ioda_group_c_create(ioda_group_t p,int64_t sz,const char *name) {
    try {
        VOID_TO_CXX(ioda::Group,p,g);
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

ioda_group_t  ioda_group_c_open(ioda_group_t p,int64_t sz,const char *name) {
    try {
        VOID_TO_CXX(ioda::Group,p,g);
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

ioda_has_attributes_t ioda_group_c_has_attributes(ioda_group_t g_p)
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

ioda_has_variables_t ioda_group_c_has_variables(ioda_group_t g_p)
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
