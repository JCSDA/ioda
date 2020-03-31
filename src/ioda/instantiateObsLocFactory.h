/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef IODA_INSTANTIATEOBSLOCFACTORY_H_
#define IODA_INSTANTIATEOBSLOCFACTORY_H_

#include "ioda/ObsLocGC99.h"
#include "oops/interface/ObsLocalization.h"

namespace ioda {
template<typename MODEL> void instantiateObsLocFactory() {
  static oops::ObsLocalizationMaker<MODEL, oops::ObsLocalization<MODEL, ioda::ObsLocGC99> >
           maker_("Gaspari-Cohn");
}

}  // namespace ioda

#endif  // IODA_INSTANTIATEOBSLOCFACTORY_H_
