/*
 * (C) Copyright 2017-2023 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#pragma once
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include "ioda/ObsGroup.h"
#include "ioda/ObsSpace.h"

extern "C"
{
    void * c_ioda_obs_space_get_group(void *obs_space_ptr);
}
