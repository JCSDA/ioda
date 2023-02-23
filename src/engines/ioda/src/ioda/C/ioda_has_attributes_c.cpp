/*
 * (C) Copyright 2022-2023 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/C/ioda_has_attributes_c.hpp"

extern "C"
{

void * ioda_has_attributes_c_alloc() {
        return reinterpret_cast<void*>(new ioda::Has_Attributes());
}

void ioda_has_attributes_c_dtor(void **v) {
        void *v_ = *v;
        VOID_TO_CXX(ioda::Has_Attributes,v_,p);
        if ( p ) {
            delete p;
        }
        *v = nullptr;
}

void *  ioda_has_attributes_c_list(void *v)
{
    try { 	
        VOID_TO_CXX(ioda::Has_Attributes,v,p);
        if ( p == nullptr) throw std::exception();
        std::vector<std::string> * vstr = new std::vector<std::string>(p->list());
        return reinterpret_cast<void*>(vstr); 
    } catch (std::exception& e) {
        std::cerr << " ioda_has_attributes_c_list failed " << e.what() << "\n";
        fatal_error();
    }    
    return 0x0;
}

void ioda_has_attributes_c_clone(void **t_p,void *rhs_p)
{
    try {
        ioda::Has_Attributes ** t = 
            reinterpret_cast< ioda::Has_Attributes ** >(t_p);
        VOID_TO_CXX(ioda::Has_Attributes,rhs_p,rhs);
        if ( *t != nullptr) {
            delete *t;    
        }
        if ( rhs == nullptr) {
            return;
        }
        *t = new ioda::Has_Attributes(*rhs);
        t_p = reinterpret_cast< void ** >(t);
        return;
    } catch ( std::exception& e) {
        std::cerr << "ioda_has_attributes_c_clone exception " << e.what() << "\n";
        fatal_error();  
    }
}

int ioda_has_attributes_c_exists(void * v,int64_t n,void *name) {
    try { 	
        VOID_TO_CXX(ioda::Has_Attributes,v,p);
        VOID_TO_CXX(const char,name,name_p);
        if ( p == nullptr) throw std::exception();
        bool ex = p->exists(std::string(name_p,n));
        return ex ? 1:0;
    } catch (std::exception& e) {
        return -1;
    }    
}

bool ioda_has_attributes_c_remove(void *v,int64_t n,void *name)
{
    try { 	
        VOID_TO_CXX(ioda::Has_Attributes,v,p);
        VOID_TO_CXX(const char,name,name_p);      
        if ( p == nullptr) throw std::exception();
        p->remove(std::string(name_p,n));
        return true;
    } catch (std::exception& e) {
        return false;
    }    
}

bool ioda_has_attributes_c_rename(void * v,int64_t old_sz,const char *old_name,int64_t new_sz,const char *new_name)
{
    try { 	
        VOID_TO_CXX(ioda::Has_Attributes,v,p);
        VOID_TO_CXX(const char,new_name,new_name_p);
        VOID_TO_CXX(const char,old_name,old_name_p);
        if ( p == nullptr) throw std::exception();
        p->rename(std::string(old_name_p,old_sz),std::string(new_name_p,new_sz));
        return true;
    } catch (std::exception& e) {
        return false;
    }    
}

void * ioda_has_attributes_c_open(void * v,int64_t n,const char *name)
{
    try { 	
        VOID_TO_CXX(ioda::Has_Attributes,v,p);
        if ( p == nullptr) throw std::exception();
        return reinterpret_cast<void*>( new ioda::Attribute( p->open(std::string(name,n)) ) );
    } catch (std::exception& e) {
        std::cerr << "ioda_has_attributes_c_open failed " << e.what() << "\n";
        fatal_error();
    }
    return nullptr;    
}

#define IODA_FUN(NAME,TYPE)\
bool ioda_has_attributes_c_create##NAME (void *v,int64_t name_sz,const char *name,	\
int64_t sz,int64_t *dims_,void **Attr) { 						\
    try {										\
        VOID_TO_CXX(ioda::Has_Attributes,v,p);  					\
        if ( p == nullptr) throw std::exception();					\
        VOID_TO_CXX(ioda::Attribute,*Attr,attr);					\
        if ( attr != nullptr ) delete attr;						\
        std::vector<ioda::Dimensions_t> vdims(dims_,dims_+sz);				\
        std::string vstr(name,name_sz);							\
        attr = new ioda::Attribute(p->create< TYPE >(vstr,vdims));			\
        *Attr = reinterpret_cast<void*>(attr);						\
        return true;									\
    } catch (std::exception& e) {							\
        std::cerr << "ioda_has_attributes_c_create failed " << e.what() << "\n";	\
    } 											\
    return false;									\
}

IODA_FUN(_float,float)
IODA_FUN(_double,double)
IODA_FUN(_char,char)
IODA_FUN(_int16,int16_t)
IODA_FUN(_int32,int32_t)
IODA_FUN(_int64,int64_t)
IODA_FUN(_str,std::vector<std::string>)
#undef IODA_FUN

}

