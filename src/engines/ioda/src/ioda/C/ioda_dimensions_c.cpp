/*
 * (C) Copyright 2022-2023 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/C/ioda_decls.hpp"
#include "ioda/C/ioda_dimensions_c.hpp"

extern "C"
{

ioda_dimensions_t  ioda_dimensions_c_alloc() {
    return reinterpret_cast<void*>(new ioda::Dimensions());
}

void ioda_dimensions_c_set(ioda_dimensions_t *v,
    int64_t ndim,int64_t n_curr_dim,int64_t n_max_dim,
    int64_t * max_dims,
    int64_t * cur_dims)
{
    ioda_dimensions_t  vp = *v;
    VOID_TO_CXX(ioda::Dimensions,vp,new_dims);    
    ioda::Dimensions_t nelem = 1;
    for (int64_t k=0;k<n_curr_dim;++k) nelem *= cur_dims[k];        
    *new_dims = ioda::Dimensions(
            std::vector<ioda::Dimensions_t>(cur_dims,cur_dims+n_curr_dim),
            std::vector<ioda::Dimensions_t>(max_dims,max_dims+n_max_dim),
            ioda::Dimensions_t(ndim),nelem);
}

void ioda_dimensions_c_dtor(ioda_dimensions_t *v) {
    if ( !v ) return;
    ioda_dimensions_t  vp = *v;
    VOID_TO_CXX(ioda::Dimensions,vp,p);
    if (p) delete p;
    p = nullptr;
}

void ioda_dimensions_c_clone(ioda_dimensions_t *t_p,ioda_dimensions_t rhs_p)
{
    try {
        ioda::Dimensions ** t = 
            reinterpret_cast< ioda::Dimensions ** >(t_p);
        VOID_TO_CXX(ioda::Dimensions,rhs_p,rhs);
        if ( *t != nullptr) {
            delete *t;    
        }
        if ( rhs == nullptr) {
            return;
        }
        *t = new ioda::Dimensions(*rhs);
        t_p = reinterpret_cast< ioda_dimensions_t * >(t);
        return;
    } catch ( std::exception& e) {
        std::cerr << "ioda_dimensions_c_clone exception " << e.what() << "\n";
        fatal_error();  
    }
}

int64_t ioda_dimensions_c_get_dimensionality(ioda_dimensions_t v) {
    VOID_TO_CXX(ioda::Dimensions,v,p);
    if ( p == nullptr ) {
        std::cerr << "ioda_dimensions_c_get_dimensionality null pointer\n";
        fatal_error();
    }
    return (p->dimensionality);
}

int64_t ioda_dimensions_c_num_of_elements(ioda_dimensions_t v) {
    VOID_TO_CXX(ioda::Dimensions,v,p);
    if ( p == nullptr ) {
        std::cerr << "ioda_dimensions_c_get_num_of_elements null pointer\n";
        fatal_error();
    }
    return (p->numElements);
}

void ioda_dimensions_c_get_dims_cur(ioda_dimensions_t v,int64_t * dims,int *ndims_) {
    try {
        VOID_TO_CXX(ioda::Dimensions,v,p);
        if (p == nullptr || dims==nullptr || ndims_ == nullptr) {
            std::cerr << "ioda_dimensions_c_get_dims_cur null ptr\n";
            throw std::exception();
        }    
        *ndims_ = (p->dimsCur).size();
        for ( auto k=0;k<*ndims_;++k) {
            dims[k] = (p->dimsCur)[k];
        }
        return;
    } catch (std::exception& e) {
        std::cerr << "ioda_dimensions_c_get_dims_cur failed\n";
        std::cerr << e.what() << "\n";
        fatal_error();
    }
}

void ioda_dimensions_c_get_dims_max(ioda_dimensions_t v,int64_t * dims,int *ndims_) {
    try {
        VOID_TO_CXX(ioda::Dimensions,v,p);
        if (p == nullptr || dims==nullptr || ndims_ == nullptr) {
            std::cerr << "ioda_dimensions_c_get_dims_max null ptr\n";
            throw std::exception();
        }    
        *ndims_ = (p->dimsMax).size();
        for ( auto k=0;k<*ndims_;++k) {
            dims[k] = (p->dimsMax)[k];
        }
        return;
    } catch (std::exception& e) {
        std::cerr << "ioda_dimensions_c_get_dims_max failed\n";
        std::cerr << e.what() << "\n";
        fatal_error();
    }
}

int64_t ioda_dimensions_c_get_dims_cur_size(ioda_dimensions_t v) {
    try {
        VOID_TO_CXX(ioda::Dimensions,v,dims);
        if (dims == nullptr) {
            std::cerr << "ioda_dimensions_c_get_dims_cur_size null ptr\n";
            throw std::exception();
        }    
        return  static_cast<int64_t>((dims->dimsCur).size());
    } catch (std::exception& e) {
        std::cerr << "ioda_dimensions_c_get_dims_cur_size failed\n";
        std::cerr << e.what() << "\n";    
        fatal_error();
    }
    // silence compiler warning
    return -1;
}

int64_t ioda_dimensions_c_get_dims_max_size(ioda_dimensions_t v) {
    try {
        VOID_TO_CXX(ioda::Dimensions,v,dims);
        if (dims == nullptr) {
            std::cerr << "ioda_dimensions_c_get_dims_max_size null ptr\n";
            throw std::exception();
        }    
        return  static_cast<int64_t>((dims->dimsMax).size());
    } catch (std::exception& e) {
        std::cerr << "ioda_dimensions_c_get_dims_max_size failed\n";
        std::cerr << e.what() << "\n";    
        fatal_error();
    }
    // silence compiler warning
    return -1;
}

}