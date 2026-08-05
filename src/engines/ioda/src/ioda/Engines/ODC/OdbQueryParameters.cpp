/*
 * (C) Crown Copyright 2021 UK Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_cxx_engines_pub_ODC
 *
 * @{
 * \file OdbQueryParameters.cpp
 * \brief ODB / ODC engine bindings
 */

#include "ioda/Engines/ODC/OdbQueryParameters.h"

namespace ioda {
namespace Engines {
namespace ODC {

constexpr char StarParameterTraitsHelper::enumTypeName[];
constexpr util::NamedEnumerator<StarParameter> StarParameterTraitsHelper::namedValues[];

void OdbVariableCreationParameters::deserialize(util::CompositePath &path,
                                                const eckit::Configuration &config) {
  Parameters::deserialize(path, config);

  if (!multichannelVarnos.value().empty() && channelIndexing.value() == boost::none)
    throw eckit::UserError(path.path() +
                           ": if the 'multichannel varnos' list is non-empty, "
                           "the 'channel indexing' option must be set");
}

}  // namespace ODC
}  // namespace Engines
}  // namespace ioda
