/*
 * (C) Copyright 2023 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "c_obsspace_get_group.hpp"

extern "C"
{
    void * c_ioda_obs_space_get_group(void *obs_space_ptr)
    {
        try {
            ioda::ObsSpace * space = reinterpret_cast< ioda::ObsSpace * >(obs_space_ptr);
            if ( space == nullptr ) {
                std::cerr << "c_ioda_obsspace_get_group error obs space ptr is null\n";
                throw std::exception();
            }
            ioda::ObsGroup * g = new ioda::ObsGroup(space->getObsGroup());
            return g;
        } catch (std::exception& e) {
            std::cerr << "c_ioda_obsspace_get_group oxception " << e.what() << "\n";
            exit(-1);
        }
        // make compiler happy this should be unreachable
        return nullptr;
    }
}
