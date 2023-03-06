/*
 * (C) Copyright 2022-2023 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/C/ioda_engines_c.hpp"

//
// passing an enum from fortran to c++ is very dangerous
//  a enum has a fixed size in fortran while in c++ that isn't true
//
//

ioda::Engines::BackendCreateModes int_to_ioda_backend_create_mode(int x)
{
    switch (x) {
    case 0:
        return ioda::Engines::BackendCreateModes::Undefined;
    case 1:
        return ioda::Engines::BackendCreateModes::Truncate_If_Exists;
    case 2:
        return ioda::Engines::BackendCreateModes::Fail_If_Exists;
    default:
        break;    
    }
    std::cerr << "undefined ioda::Engines::BackendCreateModes " << x << "\n";
    throw std::exception();
}

ioda::Engines::BackendOpenModes int_to_ioda_backend_open_mode(int x)
{
    switch (x) {
    case 0:
        return ioda::Engines::BackendOpenModes::Undefined;
    case 1:
        return ioda::Engines::BackendOpenModes::Read_Only;
    case 2:
        return ioda::Engines::BackendOpenModes::Read_Write;
    default:
        break;    
    }
    std::cerr << "undefine ioda::Engines::BackendOpenModes " << x << "\n";
    throw std::exception();
}


extern "C" {

void * ioda_engines_c_obstore_create_root_group()
{
    try {
        return reinterpret_cast<void*>(new ioda::Group(ioda::Engines::ObsStore::createRootGroup()));
    } catch (std::exception& e) {
        std::cerr << "ioda_engines_c_obstrore_create_root_group failed\n";
        std::cerr << e.what() << "\n";
        fatal_error();
    }
    return nullptr;
}

void * ioda_engines_c_hh_create_file(const void * name,int backend_mode) {
    try {
        VOID_TO_CXX(const char,name,fname);
        if (fname == nullptr || strlen(fname)==0) {
            std::cerr << "ioda_engines_c_hh_create_file null or zero filename\n";
            throw std::exception();
        }
        ioda::Engines::BackendCreateModes cmode = int_to_ioda_backend_create_mode(backend_mode);
        ioda::Group * res = new ioda::Group(
                ioda::Engines::HH::createFile(std::string(fname),cmode) );
        return reinterpret_cast<void*>(res);
    } catch (std::exception& e) {
        std::cerr << "ioda_engines_c_hh_create_file failed\n";
        std::cerr << e.what() << "\n";
        fatal_error();
    }
    return nullptr; 
}

void * ioda_engines_c_hh_open_file(const void * name,int backend_mode) {
    try {
        VOID_TO_CXX(const char,name,fname);
        if (fname == nullptr || strlen(fname)==0) {
            std::cerr << "ioda_engines_c_hh_open_file null or zero filename\n";
            throw std::exception();
        }
        ioda::Engines::BackendOpenModes omode = int_to_ioda_backend_open_mode(backend_mode);
        ioda::Group * res =  new ioda::Group( ioda::Engines::HH::openFile(std::string(fname),omode ));
        return reinterpret_cast<void*>(res);
    } catch (std::exception& e) {
        std::cerr << "ioda_engines_c_hh_open_file failed\n";
        std::cerr << e.what() << "\n";
        fatal_error();
    }
    return nullptr;
}

void * ioda_engines_c_hh_create_memory_file(const void *name,int64_t increment_len) {	
    try {
        VOID_TO_CXX(const char,name,fname);
        if (fname == nullptr || strlen(fname)==0) {
            std::cerr << "ioda_engines_c_hh_create_memory_file null or zero filename\n";
            throw std::exception();
        }
        ioda::Group * res =  new ioda::Group(ioda::Engines::HH::createMemoryFile(std::string(fname),
            ioda::Engines::BackendCreateModes::Truncate_If_Exists,false,(size_t)increment_len) );
        return reinterpret_cast<void*>(res);
    } catch (std::exception& e) {
        std::cerr << "ioda_engines_c_hh_create_memory_file failed\n";
        std::cerr << e.what() << "\n";
        fatal_error();
    }
    return nullptr;
}

void * ioda_engines_c_hh_open_memory_file(const void *name,int64_t increment_len) {	
    try {
        VOID_TO_CXX(const char,name,fname);
        if (fname == nullptr || strlen(fname)==0) {
            std::cerr << "ioda_engines_c_hh_create_memory_file null or zero filename\n";
            throw std::exception();
        }
        ioda::Group * res =  new ioda::Group(ioda::Engines::HH::openMemoryFile(std::string(fname),
            ioda::Engines::BackendOpenModes::Read_Write,false,
            (size_t)increment_len) );
        return reinterpret_cast<void*>(res);
    } catch (std::exception& e) {
        std::cerr << "ioda_engines_c_hh_create_memory_file failed\n";
        std::cerr << e.what() << "\n";
        fatal_error();
    }
    return nullptr;
}

void * ioda_engines_c_construct_from_command_line(void *vs,const void *def_name) {
    try {
        VOID_TO_CXX(const char,def_name,default_filename);
        VOID_TO_CXX(std::vector<std::string>,vs,vecstr);
        if (default_filename == nullptr) {
            std::cerr << "ioda_engines_c_construct_from_command_line filea name null char * for filename\n";
            throw std::exception();
        }
        if (strlen(default_filename)==0 ) {
            std::cerr << "ioda_engines_c_construct_from_command_line filea name is empty \n";
            throw std::exception();
        }
        // allow the passing of null pointer to indicate no comamnd line args
        if (vecstr==nullptr) {
            fprintf(stderr,"vecstring is null\n");
            int argc = 1;            
            char ** argv = new char*[1];
            argv[0] = Strdup("fort_program");
            fprintf(stderr,"argc = %d argv[0] = %s\n",argc,argv[0]);
            auto fname = std::string(default_filename);
            ioda::Group *res = new ioda::Group( ioda::Engines::constructFromCmdLine(argc,argv,fname) );
            delete argv[0];
            delete argv;            
            return reinterpret_cast<void*>(res);
        }
        //
        //  construct c command line arguments from thos of fortran
        //    if the command line arguments are
        //    ./a.out these are options
        //    for fortran argc = 3 argv[1] = these etc
        //    in  c       argc = 4 argv[0] = a.out argv[1] = these etc.
        // 
        //  out vec_string will be ( these are options ) of len = 3 
        size_t vsize = vecstr->size();
        fprintf(stderr,"vsize = %lu\n",vsize);
        int argc = static_cast<int>(vsize);
        fprintf(stderr,"argc = %d\n",argc);
        // increment by 1 to get # of arguments in c
        argc += 1;
        char ** argv = new char*[argc];
        argv[0] =  Strdup("fprog");
        for (int i=1;i<argc;++i) {
            auto as = (vecstr->at)(i-1); 
            const char * astr = as.c_str(); 
            fprintf(stderr,"%i %s\n",i,astr);
            argv[i] = Strdup(astr);
        }
        std::string fname(default_filename);
        ioda::Group *res = new ioda::Group( ioda::Engines::constructFromCmdLine(argc,argv,fname) );
        for (int i=argc;i;) {    
            --i;
            free(argv[i]);
        }
        delete [] argv;
        return reinterpret_cast<void*>(res);
    } catch (std::exception& e) {
        std::cerr << "ioda_engines_construct_from_command_line failed\n";
        std::cerr << e.what() << "\n";
        fatal_error();    
    }
    return nullptr;
}

}

