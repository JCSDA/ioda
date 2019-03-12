/*
 * (C) Copyright 2017 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef FILEIO_NETCDFIO_H_
#define FILEIO_NETCDFIO_H_

#include <string>
#include <tuple>
#include <vector>

#include <boost/any.hpp>

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
typedef std::vector<std::tuple<std::string, std::size_t>> DimListType;

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
               const int & VarSize, int * VarData);
  void ReadVar(const std::string & GroupName, const std::string & VarName,
               const int & VarSize, float * VarData);
  void ReadVar(const std::string & GroupName, const std::string & VarName,
               const int & VarSize, char * VarData);

  void WriteVar(const std::string & GroupName, const std::string & VarName,
                const int & VarSize, int * VarData);
  void WriteVar(const std::string & GroupName, const std::string & VarName,
                const int & VarSize, float * VarData);
  void WriteVar(const std::string & GroupName, const std::string & VarName,
                const int & VarSize, char * VarData);

  void ReadDateTime(uint64_t * VarDate, int * VarTime);
  void ReadDateTime(util::DateTime VarDateTime[]);

 private:
  // For the oops::Printable base class
  void print(std::ostream & os) const;

  void CheckNcCall(int RetCode, std::string & ErrorMsg);

  std::string GetNcVarName(const std::string & GroupName, const std::string & VarName);

  template <typename DataType>
  void ReadVar_helper(const std::string & GroupName, const std::string & VarName,
                      const int & VarSize, DataType * VarData);

  template <typename DataType>
  void WriteVar_helper(const std::string & GroupName, const std::string & VarName,
                       const int & VarSize, DataType * VarData);

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
   * \brief This data member holds the netcdf id of the "nlocs" dimension
   *        in the opened netcdf file.
   */
  int nlocs_id_;

  /*!
   * \brief This data member holds the netcdf id of the "nvars" dimension
   *        in the opened netcdf file.
   */
  int nvars_id_;

  /*!
   * \brief This data member holds the netcdf id of the "nrecs" dimension
   *        in the opened netcdf file.
   */
  int nrecs_id_;

  /*!
   * \brief dim_list_ dimension information from the input file
   */
  DimListType dim_list_;

};

}  // namespace ioda

#endif  // FILEIO_NETCDFIO_H_
