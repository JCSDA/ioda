/*
 * (C) Copyright 2020-2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*! \addtogroup ioda_internals_engines_obsstore
 *
 * @{
 * \file VarAttrStore.hpp
 * \brief Functions for ObsStore variable and attribute data storage
 */
#pragma once

#include <gsl/gsl-lite.hpp>
#include <string>
#include <vector>

#include "./Selection.hpp"
#include "./Types.hpp"

namespace ioda {
namespace ObsStore {
/// \ingroup ioda_internals_engines_obsstore
class VarAttrStore_Base {
private:
public:
  VarAttrStore_Base() {}
  virtual ~VarAttrStore_Base() {}

  /// \brief resizes memory allocated for data storage (vector)
  /// \param newSize new size for allocated memory in number of vector elements
  virtual void resize(std::size_t newSize) = 0;
  /// \brief resizes memory allocated for data storage (vector)
  /// \param newSize new size for allocated memory in number of vector elements
  /// \param fillvalue new elements get initialized to fillValue
  virtual void resize(std::size_t newSize, gsl::span<char> &fillValue) = 0;
  /// \brief transfer data to data storage vector
  /// \param data contiguous block of data to transfer
  /// \param m_select Selection ojbect: how to select from data argument
  /// \param f_select Selection ojbect: how to select to storage vector
  virtual void write(gsl::span<char> data, Selection &m_select, Selection &f_select) = 0;
  /// \brief transfer data from data storage vector
  /// \param data contiguous block of data to transfer
  /// \param m_select Selection ojbect: how to select to data argument
  /// \param f_select Selection ojbect: how to select from storage vector
  virtual void read(gsl::span<char> data, Selection &m_select, Selection &f_select) const = 0;
};

// Templated versions for each data type
/// \ingroup ioda_internals_engines_obsstore
template <typename DataType>
class VarAttrStore : public VarAttrStore_Base {
private:
  /// \brief data storage mechanism (vector)
  std::vector<DataType> var_attr_data_;

public:
  VarAttrStore() {}
  ~VarAttrStore() {}

  /// \brief resizes memory allocated for data storage (vector)
  /// \param newSize new size for allocated memory in number of vector elements
  void resize(std::size_t newSize) override { var_attr_data_.resize(newSize); }

  /// \brief resizes memory allocated for data storage (vector)
  /// \param newSize new size for allocated memory in number of vector elements
  /// \param fillvalue new elements get initialized to fillValue
  void resize(std::size_t newSize, gsl::span<char> &fillValue) override {
    gsl::span<DataType> fv_span(reinterpret_cast<DataType *>(fillValue.data()), 1);
    var_attr_data_.resize(newSize, fv_span[0]);
  }

  /// \brief transfer data to data storage vector
  /// \param data contiguous block of data to transfer
  /// \param m_select Selection ojbect: how to select from data argument
  /// \param f_select Selection ojbect: how to select to storage vector
  void write(gsl::span<char> data, Selection &m_select, Selection &f_select) override {
    std::size_t numObjects = data.size() / sizeof(DataType);
    gsl::span<DataType> d_span(reinterpret_cast<DataType *>(data.data()), numObjects);
    // assumes m_select and f_select have same number of points
    m_select.init_lin_indx();
    f_select.init_lin_indx();
    while (!m_select.end_lin_indx()) {
      std::size_t m_indx     = m_select.next_lin_indx();
      std::size_t f_indx     = f_select.next_lin_indx();
      var_attr_data_[f_indx] = d_span[m_indx];
    }
  }

  /// \brief transfer data from data storage vector
  /// \param data contiguous block of data to transfer
  /// \param m_select Selection ojbect: how to select to data argument
  /// \param f_select Selection ojbect: how to select from storage vector
  void read(gsl::span<char> data, Selection &m_select, Selection &f_select) const override {
    std::size_t datumLen = sizeof(DataType);
    std::size_t numChars = var_attr_data_.size() * datumLen;
    gsl::span<char> c_span(
      const_cast<char *>(reinterpret_cast<const char *>(var_attr_data_.data())), numChars);
    // assumes m_select and f_select have same number of points
    m_select.init_lin_indx();
    f_select.init_lin_indx();
    while (!m_select.end_lin_indx()) {
      std::size_t m_indx = m_select.next_lin_indx() * datumLen;
      std::size_t f_indx = f_select.next_lin_indx() * datumLen;
      for (std::size_t i = 0; i < datumLen; ++i) {
        data[m_indx + i] = c_span[f_indx + i];
      }
    }
  }
};

// Specialization for std::string data type
/// \ingroup ioda_internals_engines_obsstore
template <>
class VarAttrStore<std::string> : public VarAttrStore_Base {
private:
  /// \brief data storage mechanism (vector)
  std::vector<std::string> var_attr_data_;

public:
  VarAttrStore() {}
  ~VarAttrStore() {}

  /// \brief resizes memory allocated for data storage (vector)
  /// \param newSize new size for allocated memory in number of vector elements
  void resize(std::size_t newSize) override { var_attr_data_.resize(newSize); }

  /// \brief resizes memory allocated for data storage (vector)
  /// \param newSize new size for allocated memory in number of vector elements
  /// \param fillvalue new elements get initialized to fillValue
  void resize(std::size_t newSize, gsl::span<char> &fillValue) override {
    // At this point, fillValue[0] is a char * pointing to the string
    // to be used for a fill value.
    gsl::span<char *> fv_span(reinterpret_cast<char **>(fillValue.data()), 1);
    var_attr_data_.resize(newSize, fv_span[0]);
  }

  /// \brief transfer data to data storage vector
  /// \param data contiguous block of data to transfer
  /// \param m_select Selection object: how to select from data argument
  /// \param f_select Selection object: how to select to storage vector
  void write(gsl::span<char> data, Selection &m_select, Selection &f_select) override {
    // data is a series of char * pointing to null terminated strings
    // first place the char * values in a vector
    std::size_t numObjects = data.size() / sizeof(char *);
    gsl::span<char *> d_span(reinterpret_cast<char **>(data.data()), numObjects);
    std::vector<char *> inStrings(numObjects);
    for (std::size_t i = 0; i < numObjects; ++i) {
      inStrings[i] = d_span[i];
    }

    // assumes m_select and f_select have same number of points
    m_select.init_lin_indx();
    f_select.init_lin_indx();
    while (!m_select.end_lin_indx()) {
      std::size_t m_indx     = m_select.next_lin_indx();
      std::size_t f_indx     = f_select.next_lin_indx();
      var_attr_data_[f_indx] = inStrings[m_indx];
    }
  }

  /// \brief transfer data from data storage vector
  /// \param data contiguous block of data to transfer
  /// \param m_select Selection ojbect: how to select to data argument
  /// \param f_select Selection ojbect: how to select from storage vector
  void read(gsl::span<char> data, Selection &m_select, Selection &f_select) const override {
    // First create a vector of char * pointers to each item in var_attr_data_.
    std::size_t numObjects = var_attr_data_.size();
    std::vector<const char *> outStrings(numObjects);
    for (std::size_t i = 0; i < numObjects; ++i) {
      outStrings[i] = var_attr_data_[i].data();
    }

    std::size_t datumLen = sizeof(char *);
    std::size_t numChars = outStrings.size() * datumLen;
    gsl::span<char> c_span(reinterpret_cast<char *>(outStrings.data()), numChars);
    // assumes m_select and f_select have same number of points
    m_select.init_lin_indx();
    f_select.init_lin_indx();
    while (!m_select.end_lin_indx()) {
      std::size_t m_indx = m_select.next_lin_indx() * datumLen;
      std::size_t f_indx = f_select.next_lin_indx() * datumLen;
      for (std::size_t i = 0; i < datumLen; ++i) {
        data[m_indx + i] = c_span[f_indx + i];
      }
    }
  }
};

/// \brief factory style function to create a new templated object
/// \ingroup ioda_internals_engines_obsstore
inline VarAttrStore_Base *createVarAttrStore(ObsTypes dtype) {
  VarAttrStore_Base *newStore = nullptr;

  // Use the dtype value to determine which templated version of the data store
  // to instantiate.
  if (dtype == ObsTypes::FLOAT) {
    newStore = new VarAttrStore<float>;
  } else if (dtype == ObsTypes::DOUBLE) {
    newStore = new VarAttrStore<double>;
  } else if (dtype == ObsTypes::LDOUBLE) {
    newStore = new VarAttrStore<long double>;
  } else if (dtype == ObsTypes::SCHAR) {
    newStore = new VarAttrStore<signed char>;
  } else if (dtype == ObsTypes::SHORT) {
    newStore = new VarAttrStore<short>;
  } else if (dtype == ObsTypes::INT) {
    newStore = new VarAttrStore<int>;
  } else if (dtype == ObsTypes::LONG) {
    newStore = new VarAttrStore<long>;
  } else if (dtype == ObsTypes::LLONG) {
    newStore = new VarAttrStore<long long>;
  } else if (dtype == ObsTypes::UCHAR) {
    newStore = new VarAttrStore<unsigned char>;
  } else if (dtype == ObsTypes::USHORT) {
    newStore = new VarAttrStore<unsigned short>;
  } else if (dtype == ObsTypes::UINT) {
    newStore = new VarAttrStore<unsigned int>;
  } else if (dtype == ObsTypes::ULONG) {
    newStore = new VarAttrStore<unsigned long>;
  } else if (dtype == ObsTypes::ULLONG) {
    newStore = new VarAttrStore<unsigned long long>;
  } else if (dtype == ObsTypes::CHAR) {
    newStore = new VarAttrStore<char>;
  } else if (dtype == ObsTypes::WCHAR) {
    newStore = new VarAttrStore<wchar_t>;
  } else if (dtype == ObsTypes::CHAR16) {
    newStore = new VarAttrStore<char16_t>;
  } else if (dtype == ObsTypes::CHAR32) {
    newStore = new VarAttrStore<char32_t>;
  } else if (dtype == ObsTypes::STRING) {
    newStore = new VarAttrStore<std::string>;
  } else {
    std::string ErrMsg = std::string("ioda::ObsStore::Attribute: Unrecognized data type ")
                         + std::string("encountered during Attribute object construnction");
    throw;  // jedi_throw.add("Reason", ErrMsg);
  }

  return newStore;
}

}  // namespace ObsStore
}  // namespace ioda

/// @}
