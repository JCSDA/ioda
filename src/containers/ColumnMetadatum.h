/*
 * (C) Crown copyright 2024, Met Office
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef COLUMNMETADATUM_H
#define COLUMNMETADATUM_H

#include <cstdint>
#include <string>

class ColumnMetadatum {
 public:
  /// \brief constructor using full specification
  /// \param column name
  /// \param column data type
  /// \param column permission
  //  Non-explicit. Can be used with initlializer_list
  ColumnMetadatum(std::string, std::int8_t, std::int8_t);

  /// \brief constructor using partial specification
  /// \details The column permission is set to read-write
  /// \param column name
  /// \param column data type
  // Non-explicit. Can be used with initlializer_list
  ColumnMetadatum(std::string, std::int8_t);

  // This class uses move and copy constructor and assignment operators.
  ColumnMetadatum() = delete;  //!< Deleted default constructor

  /// \brief returns column name
  const std::string& getName() const;
  /// \brief returns column width
  const std::int16_t& getWidth() const;
  /// \brief returns column data type
  const std::int8_t& getType() const;
  /// \brief returns column permission
  const std::int8_t& getPermission() const;

  /// \brief set column name
  /// \details This function will throw an exception for a read-only column
  /// \param column name
  bool setName(const std::string&);
  /// \brief set column permission
  /// \details This function will throw an exception for a read-only column
  /// \param column permission
  bool setPermission(const std::int8_t&);

  /// \brief set the print format width for this column
  /// \details This function set the width regardless of permission since
  ///          this is only for print formatting.
  /// \param width in characters
  void setWidth(const std::int16_t&);

 private:
  /// \brief check for supported data type
  /// \param data type being tested
  /// \return if a valid type, the input data type is returned
  ///         otherwise an exception is thrown
  std::int8_t validateType(const std::int8_t&);

  /// \brief check for supported permission
  /// \param permission being tested
  /// \return if a valid permission, the input permission is returned
  ///         otherwise an exception is thrown
  std::int8_t validatePermission(const std::int8_t&);

  /// \brief column name
  std::string name_;

  /// \brief column width (for print formatting)
  std::int16_t width_;

  /// \brief column data type
  std::int8_t type_;

  /// \brief column permission (read only, read write)
  std::int8_t permission_;
};

#endif  // COLUMNMETADATUM_H
