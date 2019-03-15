/*
 * (C) Copyright 2017 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef FILEIO_NETCDFIO_H_
#define FILEIO_NETCDFIO_H_

#include <map>
#include <string>
#include <vector>

#include "fileio/IodaIO.h"
#include "oops/util/DateTime.h"
#include "oops/util/ObjectCounter.h"


////////////////////////////////////////////////////////////////////////
// Implementation of IodaIO for netcdf.
////////////////////////////////////////////////////////////////////////

// Forward declarations
namespace eckit {
  class Configuration;
}

namespace ioda {

// typedefs
typedef std::vector<std::size_t> DimIdToSizeType;
typedef std::map<std::string, std::size_t> DimNameToIdType;

/*! \brief Implementation of IodaIO for netcdf.
 *
 * \details The NetcdfIO class defines the constructor and methods for
 *          the abstract class IodaIO.
 *
 * \author Stephen Herbener (JCSDA)
 */
class NetcdfIO : public IodaIO,
                 private util::ObjectCounter<NetcdfIO> {
 public:
  /*!
   * \brief classname method for object counter
   *
   * \details This method is supplied for the ObjectCounter base class.
   *          It defines a name to identify an object of this class
   *          for reporting by OOPS.
   */
  static const std::string classname() {return "ioda::NetcdfIO";}

  NetcdfIO(const std::string & FileName, const std::string & FileMode,
           const std::size_t & Nlocs, const std::size_t & Nrecs,
           const std::size_t & Nvars);
  ~NetcdfIO();

  void ReadVar(const std::string & GroupName, const std::string & VarName,
               const std::vector<std::size_t> & VarShape, int * VarData);
  void ReadVar(const std::string & GroupName, const std::string & VarName,
               const std::vector<std::size_t> & VarShape, float * VarData);
  void ReadVar(const std::string & GroupName, const std::string & VarName,
               const std::vector<std::size_t> & VarShape, char * VarData);

  void WriteVar(const std::string & GroupName, const std::string & VarName,
                const std::vector<std::size_t> & VarShape, int * VarData);
  void WriteVar(const std::string & GroupName, const std::string & VarName,
                const std::vector<std::size_t> & VarShape, float * VarData);
  void WriteVar(const std::string & GroupName, const std::string & VarName,
                const std::vector<std::size_t> & VarShape, char * VarData);

 private:
  // For the oops::Printable base class
  void print(std::ostream & os) const;

  void CheckNcCall(int RetCode, std::string & ErrorMsg);

  std::string FormNcVarName(const std::string & GroupName, const std::string & VarName);

  void CreateNcDim(const std::string DimName, const std::size_t DimSize);

  int GetStringDimBySize(const std::size_t DimSize);

  void ReadConvertDateTime(std::string GroupName, std::string VarName, char * VarData);

  template <typename DataType>
  void ReadVar_helper(const std::string & GroupName, const std::string & VarName,
                      const std::vector<std::size_t> & VarShape, DataType * VarData);

  template <typename DataType>
  void WriteVar_helper(const std::string & GroupName, const std::string & VarName,
                       const std::vector<std::size_t> & VarShape, DataType * VarData);

  // Data members
  /*!
   * \brief netcdf file id
   *
   * \details This data member holds the file id of the open netcdf file.
   *          It gives access to the dimensions, attributes and variables in
   *          the netcdf file.
   */
  int ncid_;

  /*!
   * \brief offset time flag
   *
   * \details This data member is a flag indicating the existence of the
   *          offset time variable in the netcdf file.
   */
  bool have_offset_time_;

  /*!
   * \brief date time flag
   *
   * \details This data member is a flag indicating the existence of the
   *          date_time variable in the netcdf file.
   */
  bool have_date_time_;

  /*!
   * \brief This data member holds dimension sizes indexed by dimension id number
   */
  DimIdToSizeType dim_id_to_size_;

  /*!
   * \brief This data member holds dimension id numbers indexed by dimension name
   */
  DimNameToIdType dim_name_to_id_;

  /*!
   * \brief This data member holds dimension id numbers indexed by dimension name
   *        for extra dimensions needed for character arrays
   */
  DimNameToIdType string_dims_;
};

}  // namespace ioda

#endif  // FILEIO_NETCDFIO_H_
