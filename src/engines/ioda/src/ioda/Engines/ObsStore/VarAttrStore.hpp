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
  /// \param isFill true if setting the _FillValue attribute
  virtual void write(gsl::span<const char> data, Selection &m_select,
                     Selection &f_select, const bool isFill) = 0;
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
  /// \param isFill true if setting the _FillValue attribute
  void write(gsl::span<const char> data, Selection &m_select,
             Selection &f_select, const bool isFill) override {
    if (data.size() > 0) {
      std::size_t numObjects = data.size() / sizeof(DataType);
      gsl::span<const DataType> d_span(reinterpret_cast<const DataType *>(data.data()), numObjects);
      // assumes m_select and f_select have same number of points
      // The _FillValue attribute needs to be handled as a special case. data is a
      // char span so it relies on the correct number of bytes per element (eg, 4 for int)
      // to work properly in the loop below that transfers the data. Unfortunately,
      // this is not true when the fill value is passed in. This comes from a union
      // that is 8 bytes long so that it can accommodate double precision values (eg, double).
      // The fill value union is a single element so its length doesn't align with
      // the case where the data type is 4 bytes. Say you have an int and there are two
      // elements in the target. The fill union has the correct int value in the first 4
      // bytes and garbage in the second 4 bytes so the first element gets transferred
      // correctly but the second element gets the garbage value. To handle this in the
      // loop below, keep transferring the first element from the fill value union into
      // the target. This results in repeats of the fill in each element of the target
      // which is the desired result.
      m_select.init_lin_indx();
      f_select.init_lin_indx();
      if (isFill) {
        while (!f_select.end_lin_indx()) {
          // Repeatedly transfer the value in the fill value union
          // into the var_attr_data_ target.
          std::size_t f_indx = f_select.next_lin_indx() * num_elements_;
          for (std::size_t i = 0; i < num_elements_; ++i) {
            var_attr_data_[f_indx + i] = d_span[0];
          }
        }
      } else {
        while (!m_select.end_lin_indx()) {
          std::size_t m_indx = m_select.next_lin_indx() * num_elements_;
          std::size_t f_indx = f_select.next_lin_indx() * num_elements_;
          for (std::size_t i = 0; i < num_elements_; ++i) {
            var_attr_data_[f_indx + i] = d_span[m_indx + i];
          }
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
  /// \param isFill true if setting the _FillValue attribute
  void write(gsl::span<const char> data, Selection &m_select, Selection &f_select,
             const bool isFill) override {
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
