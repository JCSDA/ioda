/*
 * (C) Copyright 2017 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef FILEIO_NETCDFIO_H_
#define FILEIO_NETCDFIO_H_


#include <string>
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
           const util::DateTime & bgn, const util::DateTime & end,
           const double & MissingValue, const eckit::mpi::Comm & comm,
           const std::size_t & Nlocs, const std::size_t & Nobs,
           const std::size_t & Nrecs, const std::size_t & Nvars);
  ~NetcdfIO();

  void ReadVar_any(const std::string & VarName, boost::any * VarData);

  void ReadVar(const std::string & VarName, int* VarData);
  void ReadVar(const std::string & VarName, float* VarData);
  void ReadVar(const std::string & VarName, double* VarData);

  void WriteVar_any(const std::string & VarName, boost::any * VarData);
  void WriteVar(const std::string & VarName, int* VarData);
  void WriteVar(const std::string & VarName, float* VarData);
  void WriteVar(const std::string & VarName, double* VarData);

  void ReadDateTime(uint64_t* VarDate, int* VarTime);
  void ReadDateTime(util::DateTime[]);

 private:
  // For the oops::Printable base class
  void print(std::ostream & os) const;

  void CheckNcCall(int, std::string &);

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
   * \brief This data member holds the netcdf id of the "nobs" dimension
   *        in the opened netcdf file.
   */
  int nobs_id_;

  /*!
   * \brief This data member holds the netcdf id of the "nrecs" dimension
   *        in the opened netcdf file.
   */
  int nrecs_id_;

  /*!
   * \brief This data member holds the netcdf id of the "nchans" dimension
   *        in the opened netcdf file.
   */
  int nchans_id_;

  /*!
   * \brief This data member holds the netcdf id of the current dataset (variable)
   *        in the opened netcdf file.
   */
  int nc_varid_;

  /*!
   * \brief This data member is a flag that indicates the existence of the
   *        "nlocs" dimension in the opened netcdf file.
   */
  bool have_nlocs_;

  /*!
   * \brief This data member is a flag that indicates the existence of the
   *        "nvars" dimension in the opened netcdf file.
   */
  bool have_nvars_;

  /*!
   * \brief This data member is a flag that indicates the existence of the
   *        "nobs" dimension in the opened netcdf file.
   */
  bool have_nobs_;

  /*!
   * \brief This data member is a flag that indicates the existence of the
   *        "nrecs" dimension in the opened netcdf file.
   */
  bool have_nrecs_;

  /*!
   * \brief This data member is a flag that indicates the existence of the
   *        "nchans" dimension in the opened netcdf file.
   */
  bool have_nchans_;

  /*!
   * \brief This data member holds the netcdf id of the current attribute
   *        in the opened netcdf file.
   */
  int nc_attid_;

  /*!
   * \brief date (YYMMDD) in NetCDF file
   */
  std::vector<int> date_;

  /*!
   * \brief time (HHMMSS) in NetCDF file
   */
  std::vector<int> time_;
};

}  // namespace ioda

#endif  // FILEIO_NETCDFIO_H_
