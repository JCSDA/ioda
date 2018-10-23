/*
 * (C) Copyright 2018 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef IODA_ODBAPIIO_H_
#define IODA_ODBAPIIO_H_

#include "odb_api/odbql.h"

#include "oops/util/ObjectCounter.h"

#include "fileio/IodaIO.h"

////////////////////////////////////////////////////////////////////////
// Implementation of IodaIO for ODB API.
////////////////////////////////////////////////////////////////////////

// Forward declarations
namespace eckit {
  class Configuration;
}

namespace ioda {

/*! \brief Implementation of IodaIO for ODB API.
 *
 * \details The OdbOpiIO class defines the constructor and methods for
 *          the abstract class IodaIO.
 *
 * \author Steven Vahl (NCAR)
 */
class OdbApiIO : public IodaIO,
                 private util::ObjectCounter<OdbApiIO> {

 public:
   /*!
    * \brief classname method for object counter
    *
    * \details This method is supplied for the ObjectCounter base class.
    *          It defines a name to identify an object of this class
    *          for reporting by OOPS.
    */
   static const std::string classname() {return "ioda::OdbApiIO";}

   OdbApiIO(const std::string & FileName, const std::string & FileMode,
            const std::size_t & Nlocs, const std::size_t & Nobs,
            const std::size_t & Nrecs, const std::size_t & Nvars);
   ~OdbApiIO();

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

   //int CreateReadVarSQL(const std::string & VarName, int expectedType, odbql_stmt **res);
   //int RetrieveColValPtr(odbql_stmt* res, int col, odbql_value **ppv); 
   template <class T>
   void ReadVarTemplate(const std::string & VarName, T* VarData);


   // Data members
   /*!
    * \brief pointer to odbql object
    *
    * \details This data member is a pointer to a odbql type. The
    *          odbql type is returned by the odbql_open function and
    *          is used by all following odb api functions to interact with 
    *          the odb api file.
    */
   odbql *db_ = nullptr;
   //TODO:Maybe better to use unique_ptr, but causes compiler error of 
   //"incomplete type 'odbql'"
   //std::unique_ptr<odbql> db_;
   
   /*!
    * \brief pointer to odbql_stmt object used for selecting a single variable
    *
    * \details Rather than create a new odbql_stmt for every call to one of the
    *          ReadVar methods, we create a parameterized statement once for
    *          the OdbApiIO object. It is finalized in the class destructor.
    *
    *          On second thought, not sure about thread-safety of this approach. TABLE FOR NOW
    */
   //odbql_stmt *readvar_sql_ = nullptr;
};

}  // namespace ioda

#endif  // IODA_ODBAPIIO_H_
