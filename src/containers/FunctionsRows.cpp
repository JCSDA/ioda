/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#include "ioda/containers/FunctionsRows.h"

#include "ioda/containers/Constants.h"
#include "ioda/containers/Datum.h"

osdf::FunctionsRows::FunctionsRows() {}

void osdf::FunctionsRows::sortRows(osdf::IRowsData* data, const std::string& columnName,
    const std::function<std::int8_t(const std::shared_ptr<DatumBase>,
    const std::shared_ptr<DatumBase>)> func) {
  // Build list of ordered indices.
  const std::int32_t index = data->getIndex(columnName);
  const std::int64_t sizeRows = data->getSizeRows();
  const std::size_t sizeRowsSz = static_cast<std::size_t>(sizeRows);
  std::vector<std::int64_t> indices(sizeRowsSz, 0);
  std::iota(std::begin(indices), std::end(indices), 0);    // Initial sequential list of indices.
  std::sort(std::begin(indices), std::end(indices), [&](const std::int64_t& i,
                                                        const std::int64_t& j) {
    std::shared_ptr<DatumBase>& datumA = data->getDataRow(i).getColumn(index);
    std::shared_ptr<DatumBase>& datumB = data->getDataRow(j).getColumn(index);
    return func(datumA, datumB);
  });
  // Swap data values for whole rows - casting makes it look more confusing than it is.
  for (std::size_t i = 0; i < sizeRowsSz; ++i) {
    while (indices.at(i) != indices.at(static_cast<std::size_t>(indices.at(i)))) {
      const std::size_t iIdx = static_cast<std::size_t>(indices.at(i));
      std::swap(data->getDataRow(indices.at(i)), data->getDataRow(indices.at(iIdx)));
      std::swap(indices.at(i), indices.at(iIdx));
    }
  }
}

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
template void osdf::FunctionsRows::setDatumValue<std::string>(
              const std::shared_ptr<osdf::DatumBase>&, const std::string&) const;


template<typename T>
void osdf::FunctionsRows::getColumn(const osdf::IRowsData* data, const std::int32_t columnIndex,
                                    std::vector<T>& values) const {
  values.resize(static_cast<std::size_t>(data->getSizeRows()));
  for (std::int32_t rowIndex = 0; rowIndex < data->getSizeRows(); ++rowIndex) {
    const std::shared_ptr<DatumBase>& datum = data->getDataRow(rowIndex).getColumn(columnIndex);
    const T value = getDatumValue<T>(datum);
    values.at(static_cast<std::size_t>(rowIndex)) = value;
  }
}

template void osdf::FunctionsRows::getColumn<std::int8_t>(const osdf::IRowsData*,
    const std::int32_t, std::vector<std::int8_t>&) const;
template void osdf::FunctionsRows::getColumn<std::int16_t>(const osdf::IRowsData*,
    const std::int32_t, std::vector<std::int16_t>&) const;
template void osdf::FunctionsRows::getColumn<std::int32_t>(const osdf::IRowsData*,
    const std::int32_t, std::vector<std::int32_t>&) const;
template void osdf::FunctionsRows::getColumn<std::int64_t>(const osdf::IRowsData*,
    const std::int32_t, std::vector<std::int64_t>&) const;
template void osdf::FunctionsRows::getColumn<float>(const osdf::IRowsData*,
    const std::int32_t, std::vector<float>&) const;
template void osdf::FunctionsRows::getColumn<double>(const osdf::IRowsData*,
    const std::int32_t, std::vector<double>&) const;
template void osdf::FunctionsRows::getColumn<std::string>(const osdf::IRowsData*,
    const std::int32_t, std::vector<std::string>&) const;
