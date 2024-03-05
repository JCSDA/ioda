/*
 * (C) Copyright 2022-2023 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/C/ioda_has_attributes_c.hpp"

extern "C"
{

ioda_has_attributes_t ioda_has_attributes_c_alloc() {
        return reinterpret_cast<void*>(new ioda::Has_Attributes());
}

void ioda_has_attributes_c_dtor(ioda_has_attributes_t *v_p) {
        ioda::Has_Attributes * var = reinterpret_cast<ioda::Has_Attributes *>(*v_p);
        // do not delete p since it is a weak ptt to reference (will be deleted on dtor of c++ class) ?
        var = nullptr;
}

ioda_has_attributes_t  ioda_has_attributes_c_list(ioda_has_attributes_t v)
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

void ioda_has_attributes_c_clone(ioda_has_attributes_t* t_p,ioda_has_attributes_t rhs_p)
{
    try {
        if ( t_p == nullptr) {
            std::cerr << "ioda has attributes clone lhs is null\n";
        }
        VOID_TO_CXX(ioda::Has_Attributes,*t_p,lhs);
        VOID_TO_CXX(ioda::Has_Attributes,rhs_p,rhs);
        if ( rhs == nullptr) {
            std::cerr << "ioda has attributes clone rhs is null\n";
            lhs = nullptr;
            return;
        }
        /// this is weird but has_attributes need to be shallow copy
        lhs = rhs;
        *t_p = lhs;
        return;
    } catch ( std::exception& e) {
        std::cerr << "ioda_has_attributes_c_clone exception " << e.what() << "\n";
        fatal_error();  
    }
}

int ioda_has_attributes_c_exists(ioda_has_attributes_t v,int64_t n,const char  *name) {
    try { 	
        VOID_TO_CXX(ioda::Has_Attributes,v,p);
        if ( p == nullptr) throw std::exception();
        bool ex = p->exists(std::string(name,n));
        return (ex) ? 1:0;
    } catch (const std::exception& e) {
        return -1;
    }    
}

bool ioda_has_attributes_c_remove(ioda_has_attributes_t v,int64_t n,const char *name)
{
    try { 	
        VOID_TO_CXX(ioda::Has_Attributes,v,p);
        if ( p == nullptr) throw std::exception();
        p->remove(std::string(name,n));
        return true;
    } catch (std::exception& e) {
        return false;
    }    
}

bool ioda_has_attributes_c_rename(ioda_has_attributes_t v,int64_t old_sz,
    const char *old_name,int64_t new_sz,const char *new_name)
{
    try { 	
        VOID_TO_CXX(ioda::Has_Attributes,v,p);
        if ( p == nullptr) throw std::exception();
        p->rename(std::string(old_name,old_sz),std::string(new_name,new_sz));
        return true;
    } catch (std::exception& e) {
        return false;
    }    
}

ioda_has_attributes_t ioda_has_attributes_c_open(ioda_has_attributes_t v,int64_t n,const char *name)
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
bool ioda_has_attributes_c_create##NAME (ioda_has_attributes_t v,int64_t name_sz,	\
    const char *name, int64_t sz,int64_t *dims,ioda_attribute_t *Attr) { 		\
    try {										\
        VOID_TO_CXX(ioda::Has_Attributes,v,p);  					\
        if ( p == nullptr ) {								\
            std::cerr << "ioda_has_atttibute_c_create null has_attributes ptr\n"; 	\
            throw std::exception();							\
        }										\
        if ( p == nullptr) throw std::exception();					\
        VOID_TO_CXX(ioda::Attribute,*Attr,attr);					\
        if ( attr != nullptr ) {							\
             delete attr;								\
        }										\
        std::vector<ioda::Dimensions_t> vdims(dims,dims+sz);				\
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
IODA_FUN(_str,std::string)
#undef IODA_FUN

}

