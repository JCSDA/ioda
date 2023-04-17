/*
 * (C) Copyright 2022-2023 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/C/ioda_vecstring_c.hpp"

typedef void* ioda_vecstring_t;

extern "C" {

void * ioda_vecstring_c_alloc()
{
    return reinterpret_cast<void *>(new std::vector<std::string>());
}

void ioda_vecstring_c_dealloc(void **p)
{
    if ( p == nullptr) return;
    VOID_TO_CXX(std::string,*p,vs); 
    if ( vs != nullptr) {
        std::cerr << "deallocationg ptr\n";
        std::cerr << "size = " << vs->size() << "\n";
        delete vs;
    } else {
        std::cerr << "dealloc on null ptr\n";
    }
    vs = nullptr;
}

void ioda_vecstring_c_copy(void **t_p,void *rhs_p)
{
    try {
        std::vector<std::string> ** t = 
            reinterpret_cast< std::vector<std::string> ** >(t_p);
        VOID_TO_CXX(std::vector<std::string>,rhs_p,rhs);
        if ( *t != nullptr) {
            delete *t;    
        }
        if ( rhs == nullptr) {
            *t = nullptr;
            return;
        }
        *t = new std::vector<std::string>(*rhs);
        return;
    } catch ( std::exception& e) {
        std::cerr << "ioda_vecstring_c_clone exception " << e.what() << "\n";
        fatal_error();  
    }
}

void ioda_vecstring_c_set_string_f(void *p,int64_t i,void *pstr)
{
    try {
        VOID_TO_CXX(std::vector<std::string>,p,vs);
        VOID_TO_CXX(std::string,pstr,vp);
        if ( vs == nullptr || vp==nullptr) {
            std::cerr << "ioda_vecstring_c_set_string_f null pointer in argument\n";
            throw std::exception();
        }
        (vs->at)((i-1)) = *vp;
    } catch (std::exception& e) {
        std::cerr << "ioda_vecstring_c_set_string_f failed " << e.what() << "\n";
        fatal_error();
    }    
}

void ioda_vecstring_c_append_string_f(void *p,int64_t i,void *pstr)
{
    try {
        VOID_TO_CXX(std::vector<std::string>,p,vs);
        VOID_TO_CXX(std::string,pstr,vp);
        if ( vs == nullptr || vp==nullptr) {
            std::cerr << "ioda_vecstring_c_append_string_f null pointer in argument\n";
            throw std::exception();
        }
        (vs->at)((i-1)) += *vp;
    } catch (std::exception& e) {
        std::cerr << "ioda_vecstring_c_append_string_f failed " << e.what() << "\n";
        fatal_error();
    }    
}

void * ioda_vecstring_c_get_string_f(void *p,int64_t i)
{
    try {
        VOID_TO_CXX(std::vector<std::string>,p,vs);
        if ( vs == nullptr) {
            std::cerr << "ioda_vecstring_c_get_string_f null pointer in argument\n";
            throw std::exception();
        }
       return reinterpret_cast<void*>( new std::string( vs->at(i-1) ));
    } catch (std::exception& e) {
        std::cerr << "ioda_vecstring_c_get_string_f failed " << e.what() << "\n";
        fatal_error();
    }    
/// quiet compiler warning 
    return 0x0;
}

void  ioda_vecstring_c_set_f(void *p,int64_t i,void *pstr)
{
    try {
        VOID_TO_CXX(std::vector<std::string>,p,vs);
        VOID_TO_CXX(const char,pstr,str);
        if ( vs == nullptr || str == nullptr) {
            std::cerr << "ioda_vecstring_c_get_f null pointer in argument\n";
            throw std::exception();
        }
        --i;
        vs->at(i) = std::string(str);
    } catch (std::exception& e) {
        std::cerr << "ioda_vecstring_c_get_f failed " << e.what() << "\n";
        fatal_error();
    }    
}

void  ioda_vecstring_c_append_f(void *p,int64_t i,void *pstr)
{
    try {
        VOID_TO_CXX(std::vector<std::string>,p,vs);
        VOID_TO_CXX(const char,pstr,str);
        if ( vs == nullptr || str == nullptr) {
            std::cerr << "ioda_vecstring_c_app_f null pointer in argument\n";
            throw std::exception();
        }
        --i;
        vs->at(i) += std::string(str);
    } catch (std::exception& e) {
        std::cerr << "ioda_vecstring_c_app_f failed " << e.what() << "\n";
        fatal_error();
    }    
}

char * ioda_vecstring_c_get_f(void *p,int64_t i)
{
    try {
        VOID_TO_CXX(std::vector<std::string>,p,vs);
        if ( vs == nullptr) {
            std::cerr << "ioda_vecstring_c_get_f null pointer in argument\n";
            throw std::exception();
        }
        --i;
        std::string sx{(vs->at)(i)};
        return Strdup(sx.c_str());
    } catch (std::exception& e) {
        std::cerr << "ioda_vecstring_c_get_f failed " << e.what() << "\n";
        fatal_error();
    }    
/// quiet compiler warning 
    return 0x0;
}

void ioda_vecstring_c_clear(void *p) {
    try {
        VOID_TO_CXX(std::vector<std::string>,p,vs);
        if ( vs == nullptr) {
            std::cerr << "ioda_vecstring_c_clear pointer in argument\n";
            throw std::exception();
        }
        vs->clear();
    } catch (std::exception& e) {
        std::cerr << "ioda_vecstring_c_clear failed " << e.what() << "\n";
        fatal_error();
    }    
}

void ioda_vecstring_c_resize(void *p,int64_t n) {
    try {
        VOID_TO_CXX(std::vector<std::string>,p,vs);
        if ( vs == nullptr) {
            std::cerr << "ioda_vecstring_c_resize pointer in argument\n";
            throw std::exception();
        }
        vs->resize(n);
    } catch (std::exception& e) {
        std::cerr << "ioda_vecstring_c_resize failed " << e.what() << "\n";
        fatal_error();
    }    
}


int64_t ioda_vecstring_c_size(void *p) {
    try {
        VOID_TO_CXX(std::vector<std::string>,p,vs);
        if ( vs == nullptr) {
            std::cerr << "ioda_vecstring_c_size pointer in argument\n";
            throw std::exception();
        }
        return static_cast<int64_t>( vs->size() );
    } catch (std::exception& e) {
        std::cerr << "ioda_vecstring_c_size failed " << e.what() << "\n";
        fatal_error();
    }    
/// quiet compiler warning 
    return -1;
}

int64_t ioda_vecstring_c_element_size_f(void *p,int64_t i) {
    try {
        VOID_TO_CXX(std::vector<std::string>,p,vs);
        if ( vs == nullptr) {
            std::cerr << "ioda_vecstring_c_element_size pointer in argument\n";
            throw std::exception();
        }
        --i;
        const std::string& vi = vs->at(i);
        return static_cast<int64_t>(vi.size());
    } catch (std::exception& e) {
        std::cerr << "ioda_vecstring_c_element_size failed " << e.what() << "\n";
        fatal_error();
    }    
/// quiet compiler warning 
    return -1;
}

void ioda_vecstring_c_push_back(void *p,void *pstr) {
    try {
        VOID_TO_CXX(std::vector<std::string>,p,vs);
        VOID_TO_CXX(const char,pstr,str);
        if ( vs == nullptr || str == nullptr) {
            std::cerr << "ioda_vecstring_c_push_back pointer in argument\n";
            throw std::exception();
        }
        vs->push_back(std::string(str));
    } catch (std::exception& e) {
        std::cerr << "ioda_vecstring_c_push_back failed " << e.what() << "\n";
        fatal_error();
    }    
}

void ioda_vecstring_c_push_back_string(void *p,void *str) {
    try {
        VOID_TO_CXX(std::vector<std::string>,p,vs);
        VOID_TO_CXX(std::string,p,str);
        if ( vs == nullptr || str == nullptr) {
            std::cerr << "ioda_vecstring_c_push_back_string pointer in argument\n";
            throw std::exception();
        }
        vs->push_back(*str);
    } catch (std::exception& e) {
        std::cerr << "ioda_vecstring_c_push_back_string failed " << e.what() << "\n";
        fatal_error();
    }    
}

void * ioda_string_c_alloc()
{
    try {
        return reinterpret_cast<void*>(new std::string);
    } catch (std::exception& e) {
        std::cerr << "ioda_string failed_alloc " << e.what() << "\n";
        fatal_error();
    }    
// quiet compiler warning this cannot be reached
    return 0x0;
}

void ioda_string_c_dealloc(void *p)
{
    VOID_TO_CXX(std::string,p,s);
    if (s) delete s;
    s = nullptr;
}

void ioda_string_c_copy(void **t_p,void *rhs_p)
{
    try {
        std::string ** t = 
            reinterpret_cast< std::string ** >(t_p);
        VOID_TO_CXX(std::string,rhs_p,rhs);
        if (t == nullptr) {
            std::cerr << "ioda_string_clone nullptr !\n";
            throw std::exception();
        }
        if ( *t != nullptr) {
            delete *t;    
        }
        if ( rhs == nullptr) {
            return;
        }
        *t = new std::string(*rhs);
        t_p = reinterpret_cast< void ** >(t);
        return;
    } catch ( std::exception& e) {
        std::cerr << "ioda_string_c_clone exception " << e.what() << "\n";
        fatal_error();  
    }
}

void ioda_string_c_set(void *p,void *pstr)
{
    try {
        VOID_TO_CXX(std::string,p,s);
        VOID_TO_CXX(const char,pstr,cstr);
        if (cstr == nullptr || s == nullptr) {
            std::cerr << "ioda_string_c_set nullptr in arguments\n";
            throw std::exception();
        }
        *s = std::string(cstr);
    } catch (std::exception& e) {
        std::cerr << "ioda_string_c_set failed " << e.what() << "\n";
        fatal_error();
    }
}

void ioda_string_c_set_string(void *p,void *pstr)
{
    try {
        VOID_TO_CXX(std::string,p,s);
        VOID_TO_CXX(const std::string,pstr,str);
        if ( str == nullptr || s == nullptr) {
            std::cerr << "ioda_string_c_set_string nullptr in arguments\n";
            throw std::exception();
        }
        *s = *str;
    }  catch (std::exception& e) {
        std::cerr << "ioda_string_c_set_string failed " << e.what() << "\n";
        fatal_error();
    }
}

void ioda_string_c_append(void *p,void *pstr)
{
    try {
        VOID_TO_CXX(std::string,p,s);
        VOID_TO_CXX(const char,pstr,cstr);
        if (cstr == nullptr || s == nullptr) {
            std::cerr << "ioda_string_c_append nullptr in arguments\n";
            throw std::exception();
        }
        *s += std::string(cstr);
    }  catch (std::exception& e) {
        std::cerr << "ioda_string_c_append failed " << e.what() << "\n";
        fatal_error();
    }
}

void ioda_string_c_append_string(void *p,void *pstr)
{
    try {
        VOID_TO_CXX(std::string,p,s);
        VOID_TO_CXX(const std::string,pstr,str);
        if (str == nullptr || s == nullptr) {
            std::cerr << "ioda_string_c_append_string nullptr in arguments\n";
            throw std::exception();
        }
        *s += *str;
    }  catch (std::exception& e) {
        std::cerr << "ioda_string_c_append_string failed " << e.what() << "\n";
        fatal_error();
    }
}

char * ioda_string_c_get(void *p) {
    try {
        VOID_TO_CXX(std::string,p,s);
        if (s == nullptr) {
            std::cerr << "ioda_string_c_get nullptr in arguments\n";
            throw std::exception();
        }
        char * r = Strdup(s->c_str());
        return r;
    }  catch (std::exception& e) {
        std::cerr << "ioda_string_c_get failed " << e.what() << "\n";
        fatal_error();
    }
/// quiet compiler warning 
    return 0x0;
}

int64_t ioda_string_c_size(void *p) {
    try {
        VOID_TO_CXX(std::string,p,s);
        if (s == nullptr) {
            std::cerr << "ioda_string_c_size nullptr in arguments\n";
            throw std::exception();
        }
        return static_cast<int64_t>( s->size() );
    }  catch (std::exception& e) {
        std::cerr << "ioda_string_c__size failed " << e.what() << "\n";
        fatal_error();
    }
/// quiet compiler warning 
    return -1;
}


void ioda_string_c_clear(void *p) {
    try {
        VOID_TO_CXX(std::string,p,s);
        if (s == nullptr) {
            std::cerr << "ioda_string_c_size nullptr in arguments\n";
            throw std::exception();
        }
        s->clear();
    }  catch (std::exception& e) {
        std::cerr << "ioda_string_c_cleat failed " << e.what() << "\n";
        fatal_error();
    }
}

} // end of extern "C"
