/*
 * (C) Copyright 2020 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef CORE_INSTANTIATEOBSLOCFACTORY_H_
#define CORE_INSTANTIATEOBSLOCFACTORY_H_

#include "ioda/core/ObsLocGC99.h"
#include "oops/interface/ObsLocalization.h"

namespace ioda {
template<typename MODEL, typename OBS> void instantiateObsLocFactory() {
  static oops::ObsLocalizationMaker<MODEL, OBS,
                                    oops::ObsLocalization<MODEL, OBS, ioda::ObsLocGC99<MODEL>> >
           maker_("Gaspari-Cohn");
}

}  // namespace ioda

#endif  // CORE_INSTANTIATEOBSLOCFACTORY_H_
