/*
 * (C) Copyright 2023 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#include "ioda/c_obsdatavector_int.hpp"

extern "C" {

const int * obsdatavector_int_c_get_row_i(obsdatavector_int_t p, size_t i) {
    try {
        VOID_TO_CXX(ioda::ObsDataVector<int>, p, ovec);
        if ( ovec == nullptr) throw std::invalid_argument("obsdatavector ptr is null");
        const std::vector<int>& v = (*ovec)[i];
        return v.data();
    } catch (std::exception& e) {
        std::cerr << __FUNCTION__ << " exception " << e.what() << "\n";
        fatal_error();
    }
}

const int * obsdatavector_int_c_get_row_cxx_str(obsdatavector_int_t p, cxx_string_t str) {
    try {
        VOID_TO_CXX(ioda::ObsDataVector<int>, p, ovec);
        if ( ovec == nullptr) throw std::invalid_argument("obsdatavector ptr is null");
        VOID_TO_CXX(std::string, str, cxx_str);
        if ( cxx_str == nullptr) throw std::invalid_argument("string ptr is null");
        const std::vector<int>& v = (*ovec)[*cxx_str];
        return v.data();
    } catch (std::exception& e) {
        std::cerr << __FUNCTION__ << " exception " << e.what() << "\n";
        fatal_error();
    }
}

const int * obsdatavector_int_c_get_row_str(obsdatavector_int_t p, const char* cstr) {
    try {
        VOID_TO_CXX(ioda::ObsDataVector<int>, p, ovec);
        if ( ovec == nullptr) throw std::invalid_argument("obsdatavector ptr is null");
        if ( cstr == nullptr) throw std::invalid_argument("string ptr is null");
        const std::vector<int>& v = (*ovec)[std::string(cstr)];
        return v.data();
    } catch (std::exception& e) {
        std::cerr << __FUNCTION__ << " exception " << e.what() << "\n";
        fatal_error();
    }
}

int obsdatavector_int_c_get(obsdatavector_int_t p, int64_t i, int64_t j) {
    try {
        VOID_TO_CXX(ioda::ObsDataVector<int>, p, ovec);
        if ( ovec == nullptr) throw std::invalid_argument("obsdatavector ptr is null");
        return  ((*ovec)[i])[j];
    } catch (std::exception& e) {
        std::cerr << __FUNCTION__ << " exception " << e.what() << "\n";
        fatal_error();
    }
}

int64_t obsdatavector_int_c_nvars(obsdatavector_int_t p) {
    try {
        VOID_TO_CXX(ioda::ObsDataVector<int>, p, ovec);
        if ( ovec == nullptr) throw std::invalid_argument("obsdatavector ptr is null");
        return static_cast<int64_t>(ovec->nvars());
    } catch (std::exception& e) {
        std::cerr << __FUNCTION__ << " exception " << e.what() << "\n";
        fatal_error();
    }
}

int64_t obsdatavector_int_c_nlocs(obsdatavector_int_t p) {
    try {
        VOID_TO_CXX(ioda::ObsDataVector<int>, p, ovec);
        if ( ovec == nullptr) throw std::invalid_argument("obsdatavector ptr is null");
        return static_cast<int64_t>(ovec->nlocs());
    } catch (std::exception& e) {
        std::cerr << __FUNCTION__ << " exception " << e.what() << "\n";
        fatal_error();
    }
}
}
