/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_c_ex
 *
 * @{
 *
 * \defgroup ioda_c_ex_3 Ex 3: Introduction to Variables
 * \brief Basic usage of Variables using the C interface
 * \details This example parallels the C++ examples.
 * \see 03-VariablesIntro.cpp for comments and the walkthrough.
 *
 * @{
 *
 * \file 03-VariablesIntro.c
 * \brief Basic usage of Variables using the C interface
 * \see 03-VariablesIntro.cpp for comments and the walkthrough.
 **/
#include <cstdlib>
#include <cstdio>
#include <string>

#include "ioda/C/ioda_has_variables_c.hpp"
#include "ioda/C/ioda_variable_c.hpp"
#include "ioda/C/ioda_group_c.hpp"
#include "ioda/C/ioda_engines_c.hpp"
#include "ioda/C/ioda_vecstring_c.hpp"


int main(int argc,char **argv)
{
    void * gvars = 0x0;
    void * var1 = 0x0;
    void * grp = 0x0;
    void * var2 = 0x0;
    void * var3 = 0x0;
    void * i_dims = 0x0;
    int64_t cdims[4];
    int64_t mdims[4];
    int data_i[6];
    int data_r[6];
    char name[32];


    grp = ioda_engines_c_construct_from_command_line(0x0,"Example-03-c.hdf5");
    
    gvars = ioda_group_c_has_variables(grp);
    
    cdims[0] = 2;
    cdims[1] = 3;
    cdims[2] = 0;
    cdims[3] = 0;    
    var1 = ioda_has_variables_c_create_int32(gvars,5,"var-1",2,(void**)&cdims);

    if ( !ioda_has_variables_c_exists(gvars,5,"var-1") ) {
        fprintf(stderr,"ioda_has_variables_c_exists failed?\n");
        exit(-1);
    }
    return 0;
}    
