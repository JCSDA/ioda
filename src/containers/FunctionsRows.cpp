/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/containers/FunctionsRows.h"

#include "ioda/containers/Datum.h"

osdf::FunctionsRows::FunctionsRows() {}

template<typename T>
const T osdf::FunctionsRows::getDatumValue(const std::shared_ptr<DatumBase>& datum) const {
  const std::shared_ptr<Datum<T>>& datumType = std::static_pointer_cast<Datum<T>>(datum);
  return datumType->getValue();
}

template const std::int8_t osdf::FunctionsRows::getDatumValue<std::int8_t>(
                                                const std::shared_ptr<DatumBase>&) const;
template const std::int16_t osdf::FunctionsRows::getDatumValue<std::int16_t>(
                                                const std::shared_ptr<DatumBase>&) const;
template const std::int32_t osdf::FunctionsRows::getDatumValue<std::int32_t>(
                                                const std::shared_ptr<DatumBase>&) const;
template const std::int64_t osdf::FunctionsRows::getDatumValue<std::int64_t>(
                                                const std::shared_ptr<DatumBase>&) const;
template const float osdf::FunctionsRows::getDatumValue<float>(
                                                const std::shared_ptr<DatumBase>&) const;
template const double osdf::FunctionsRows::getDatumValue<double>(
                                                const std::shared_ptr<DatumBase>&) const;
template const char osdf::FunctionsRows::getDatumValue<char>(
                                                const std::shared_ptr<DatumBase>&) const;
template const std::string osdf::FunctionsRows::getDatumValue<std::string>(
                                                const std::shared_ptr<DatumBase>&) const;

template<typename T> void osdf::FunctionsRows::setDatumValue(
                          const std::shared_ptr<osdf::DatumBase>& datum, const T& value) const {
  const std::shared_ptr<Datum<T>>& datumType = std::static_pointer_cast<Datum<T>>(datum);
  datumType->setValue(value);
}

template void osdf::FunctionsRows::setDatumValue<std::int8_t>(
              const std::shared_ptr<osdf::DatumBase>&, const std::int8_t&) const;
template void osdf::FunctionsRows::setDatumValue<std::int16_t>(
              const std::shared_ptr<osdf::DatumBase>&, const std::int16_t&) const;
template void osdf::FunctionsRows::setDatumValue<std::int32_t>(
              const std::shared_ptr<osdf::DatumBase>&, const std::int32_t&) const;
template void osdf::FunctionsRows::setDatumValue<std::int64_t>(
              const std::shared_ptr<osdf::DatumBase>&, const std::int64_t&) const;
template void osdf::FunctionsRows::setDatumValue<float>(
              const std::shared_ptr<osdf::DatumBase>&, const float&) const;
template void osdf::FunctionsRows::setDatumValue<double>(
              const std::shared_ptr<osdf::DatumBase>&, const double&) const;
template void osdf::FunctionsRows::setDatumValue<char>(
              const std::shared_ptr<osdf::DatumBase>&, const char&) const;
template void osdf::FunctionsRows::setDatumValue<std::string>(
              const std::shared_ptr<osdf::DatumBase>&, const std::string&) const;
