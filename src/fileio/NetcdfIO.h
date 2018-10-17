/*
 * (C) Copyright 2017 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef IODA_NETCDFIO_H_
#define IODA_NETCDFIO_H_

#include "ncFile.h"

#include "oops/util/ObjectCounter.h"

#include "fileio/IodaIO.h"

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
            const std::size_t & Nlocs, const std::size_t & Nobs,
            const std::size_t & Nrecs, const std::size_t & Nvars);
   ~NetcdfIO();

   void ReadVar(const std::string & VarName, int* VarData);
   void ReadVar(const std::string & VarName, float* VarData);
   void ReadVar(const std::string & VarName, double* VarData);

   void WriteVar(const std::string & VarName, int* VarData);
   void WriteVar(const std::string & VarName, float* VarData);
   void WriteVar(const std::string & VarName, double* VarData);

   void ReadDateTime(int* VarDate, int* VarTime);

 private:
   // For the oops::Printable base class
   void print(std::ostream & os) const;

   // Data members
   /*!
    * \brief pointer to netcdf file object
    *
    * \details This data member is a pointer to a netCDF::NcFile type. The
    *          netCDF::NcFile type is returned by the file open method and
    *          gives access to the dimensions, attributes and variables in
    *          the netcdf file.
    */
   std::unique_ptr<netCDF::NcFile> ncfile_;
};

}  // namespace ioda

#endif  // IODA_NETCDFIO_H_
