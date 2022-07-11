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

#include <string>
#include <vector>

#include "gsl/gsl-lite.hpp"

#include "./Selection.hpp"
#include "./Type.hpp"
#include "ioda/Exception.h"

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
  virtual void write(gsl::span<const char> data, Selection &m_select, Selection &f_select) = 0;
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

  /// \brief number of elements in one data piece (for arrayed types)
  std::size_t num_elements_;

public:
  VarAttrStore() : num_elements_(1) {}
  VarAttrStore(const std::size_t numElements) : num_elements_(numElements) {}
  ~VarAttrStore() {}

  /// \brief resizes memory allocated for data storage (vector)
  /// \param newSize new size for allocated memory in number of vector elements
  void resize(std::size_t newSize) override { var_attr_data_.resize(newSize * num_elements_); }

  /// \brief resizes memory allocated for data storage (vector)
  /// \param newSize new size for allocated memory in number of vector elements
  /// \param fillvalue new elements get initialized to fillValue
  void resize(std::size_t newSize, gsl::span<char> &fillValue) override {
    gsl::span<DataType> fv_span(reinterpret_cast<DataType *>(fillValue.data()), 1);
    var_attr_data_.resize(newSize * num_elements_, fv_span[0]);
  }

  /// \brief transfer data to data storage vector
  /// \param data contiguous block of data to transfer
  /// \param m_select Selection ojbect: how to select from data argument
  /// \param f_select Selection ojbect: how to select to storage vector
  void write(gsl::span<const char> data, Selection &m_select, Selection &f_select) override {
    if (data.size() > 0) {
      std::size_t numObjects = data.size() / sizeof(DataType);
      gsl::span<const DataType> d_span(reinterpret_cast<const DataType *>(data.data()), numObjects);
      // assumes m_select and f_select have same number of points
      m_select.init_lin_indx();
      f_select.init_lin_indx();
      while (!m_select.end_lin_indx()) {
        std::size_t m_indx     = m_select.next_lin_indx() * num_elements_;
        std::size_t f_indx     = f_select.next_lin_indx() * num_elements_;
        for (std::size_t i = 0; i < num_elements_; ++i) {
          var_attr_data_[f_indx + i] = d_span[m_indx + i];
        }
      }
    }
  }

  /// \brief transfer data from data storage vector
  /// \param data contiguous block of data to transfer
  /// \param m_select Selection ojbect: how to select to data argument
  /// \param f_select Selection ojbect: how to select from storage vector
  void read(gsl::span<char> data, Selection &m_select, Selection &f_select) const override {
    if (data.size() > 0) {
      std::size_t numChars = var_attr_data_.size() * sizeof(DataType);
      gsl::span<char> c_span(
        const_cast<char *>(reinterpret_cast<const char *>(var_attr_data_.data())), numChars);
      // assumes m_select and f_select have same number of points
      std::size_t datumLen = num_elements_ * sizeof(DataType);
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
  }
};

// Specialization for std::string data type
/// \ingroup ioda_internals_engines_obsstore
template <>
class VarAttrStore<std::string> : public VarAttrStore_Base {
private:
  /// \brief data storage mechanism (vector)
  std::vector<std::string> var_attr_data_;

  /// \brief number of elements in one data piece (for arrayed types)
  std::size_t num_elements_;

public:
  VarAttrStore() : num_elements_(1) {}
  VarAttrStore(const std::size_t numElements) : num_elements_(numElements) {}
  ~VarAttrStore() {}

  /// \brief resizes memory allocated for data storage (vector)
  /// \param newSize new size for allocated memory in number of vector elements
  void resize(std::size_t newSize) override { var_attr_data_.resize(newSize * num_elements_); }

  /// \brief resizes memory allocated for data storage (vector)
  /// \param newSize new size for allocated memory in number of vector elements
  /// \param fillvalue new elements get initialized to fillValue
  void resize(std::size_t newSize, gsl::span<char> &fillValue) override {
    // At this point, fillValue[0] is a char * pointing to the string
    // to be used for a fill value.
    gsl::span<char *> fv_span(reinterpret_cast<char **>(fillValue.data()), 1);
    var_attr_data_.resize(newSize * num_elements_, fv_span[0]);
  }

  /// \brief transfer data to data storage vector
  /// \param data contiguous block of data to transfer
  /// \param m_select Selection object: how to select from data argument
  /// \param f_select Selection object: how to select to storage vector
  void write(gsl::span<const char> data, Selection &m_select, Selection &f_select) override {
    // data is a series of char * pointing to null terminated strings
    // first place the char * values in a vector
    if (data.size() > 0) {
      std::size_t numObjects = data.size() / sizeof(char *);
      auto data_pointer = reinterpret_cast<const char* const*>(data.data());
      std::vector<char *> inStrings(numObjects);
      for (std::size_t i = 0; i < numObjects; ++i) {
        inStrings[i] = const_cast<char*>(data_pointer[i]);
      }

      // assumes m_select and f_select have same number of points
      m_select.init_lin_indx();
      f_select.init_lin_indx();
      while (!m_select.end_lin_indx()) {
        std::size_t m_indx     = m_select.next_lin_indx() * num_elements_;
        std::size_t f_indx     = f_select.next_lin_indx() * num_elements_;
        for (std::size_t i = 0; i < num_elements_; ++i) {
          var_attr_data_[f_indx + i] = inStrings[m_indx + i];
        }
      }
    }
  }

  /// \brief transfer data from data storage vector
  /// \param data contiguous block of data to transfer
  /// \param m_select Selection ojbect: how to select to data argument
  /// \param f_select Selection ojbect: how to select from storage vector
  void read(gsl::span<char> data, Selection &m_select, Selection &f_select) const override {
    // First create a vector of char * pointers to each item in var_attr_data_.
    if (data.size() > 0) {
      std::size_t numObjects = var_attr_data_.size();
      std::vector<const char *> outStrings(numObjects);
      for (std::size_t i = 0; i < numObjects; ++i) {
        outStrings[i] = var_attr_data_[i].data();
      }

      std::size_t numChars = outStrings.size() * sizeof(char *);
      gsl::span<char> c_span(reinterpret_cast<char *>(outStrings.data()), numChars);
      // assumes m_select and f_select have same number of points
      std::size_t datumLen = num_elements_ * sizeof(char *);
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
  }
};

/// \brief factory style function to create a new templated object
/// \ingroup ioda_internals_engines_obsstore
VarAttrStore_Base *createVarAttrStore(const std::shared_ptr<Type> & dtype);

}  // namespace ObsStore
}  // namespace ioda

/// @}
