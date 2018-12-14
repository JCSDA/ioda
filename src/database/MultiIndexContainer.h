/*
 * (C) Copyright 2017 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef DATABASE_MULTIINDEXCONTAINER_H_
#define DATABASE_MULTIINDEXCONTAINER_H_

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <boost/any.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/range/iterator_range.hpp>

#include "eckit/mpi/Comm.h"

#include "oops/util/abor1_cpp.h"
#include "oops/util/DateTime.h"
#include "oops/util/Logger.h"
#include "oops/util/Printable.h"

using boost::multi_index::composite_key;
using boost::multi_index::indexed_by;
using boost::multi_index::member;
using boost::multi_index::multi_index_container;
using boost::multi_index::ordered_non_unique;
using boost::multi_index::ordered_unique;
using boost::multi_index::tag;

namespace ioda {

class ObsSpaceContainer: public util::Printable {
 public:
     explicit ObsSpaceContainer(const eckit::Configuration &);
     ~ObsSpaceContainer();

     struct by_group {};     // Index by group name
     struct by_variable {};  // Index by variable name

     struct Texture {  // Record texture
         std::string group;           /*!< Group name: ObsValue, HofX, MetaData, ObsErr etc. */
         std::string variable;        /*!< Variable name */
         std::string mode;            /*!< Read & write mode : 'r' or 'rw'*/

         Texture(const std::string & group, const std::string & variable,
                 const std::string & mode)
            : group(group), variable(variable), mode(mode){}
     };  // end of record texture defition

     struct Record : public Texture {  // Record
         std::size_t size;                    /*!< Vector size */
         std::unique_ptr<boost::any[]> data;  /*!< Smart pointer to vector */

         // Constructor with default read & write mode : "rw"
         Record(const std::string & group, const std::string & variable,
                const std::size_t & size, std::unique_ptr<boost::any[]> & vect)
            : Texture(group, variable, "rw"),
              size(size), data(std::move(vect)) {}

         // Constructor with passed read & write mode
         Record(const std::string & group, const std::string & variable,
                const std::string & mode,  const std::size_t & size,
                std::unique_ptr<boost::any[]> & vect)
            : Texture(group, variable, mode),
              size(size), data(std::move(vect)) {}
     };  // end of Record definition

     // -----------------------------------------------------------------------------
     using Record_set = multi_index_container<
         Record,
         indexed_by<
             ordered_unique<
                composite_key<
                    Record,
                    member<Texture, std::string, &Texture::group>,
                    member<Texture, std::string, &Texture::variable>
                >
             >,
             // non-unique as there are many Records under group
             ordered_non_unique<
                tag<by_group>,
                member<Texture, std::string, &Texture::group>
             >,
             // non-unique as there are Records with the same name in different group
             ordered_non_unique<
                tag<by_variable>,
                member<Texture, std::string, &Texture::variable>
             >
         >
     >;

     // -----------------------------------------------------------------------------

     /*! \brief Initialize from file*/
     void CreateFromFile(const std::string & filename, const std::string & mode,
                         const util::DateTime & bgn, const util::DateTime & end,
                         const double & missingvalue, const eckit::mpi::Comm & comm);

     /*! \brief Load VALID variables from file to container */
     void LoadData();

     /*! \brief Check the availability of Record with group and variable in container*/
     bool has(const std::string & group, const std::string & variable) const;

     /*! \brief Return the number of uniqure observation locations on this PE*/
     std::size_t nlocs() const {return nlocs_;}

     /*! \brief Return the number of observational variables*/
     std::size_t nvars() const {return nvars_;}

     // -----------------------------------------------------------------------------

     /*! \brief Inquire the vector of Record from container*/
     template <typename Type>
     void inquire(const std::string & group, const std::string & variable,
                  const std::size_t vsize, Type vdata[]) const {
       if (has(group, variable)) {  // Found the required record in database
         auto var = DataContainer.find(boost::make_tuple(group, variable));
         const std::type_info & typeInput = var->data.get()->type();
         const std::type_info & typeOutput = typeid(Type);

         if ((typeInput == typeid(float)) && (typeOutput == typeid(double))) {
           oops::Log::warning() << " DataContainer::inquire: inconsistent type : "
                                << " From float to double on "
                                << variable << " : " << group << std::endl;
           for (std::size_t ii = 0; ii < vsize; ++ii)
             vdata[ii] = static_cast<double>(boost::any_cast<float>(var->data.get()[ii]));
         } else if ((typeInput == typeid(double)) && (typeOutput == typeid(int))) {
             oops::Log::warning() << " DataContainer::inquire: inconsistent type : "
                                  << " From double to int on "
                                  << variable << " : " << group << std::endl;
            for (std::size_t ii = 0; ii < vsize; ++ii)
              vdata[ii] = static_cast<int>(boost::any_cast<double>(var->data.get()[ii]));
         } else if ((typeInput == typeid(int)) && (typeOutput == typeid(double))) {
             oops::Log::warning() << " DataContainer::inquire: inconsistent type : "
                                  << " From int to double on "
                                  << variable << " : " << group << std::endl;
            for (std::size_t ii = 0; ii < vsize; ++ii)
              vdata[ii] = static_cast<double>(boost::any_cast<int>(var->data.get()[ii]));
         } else {  // For most of the cases, the in/out types should be the same
           ASSERT(typeInput == typeOutput);
           for (std::size_t ii = 0; ii < vsize; ++ii)
             vdata[ii] = boost::any_cast<Type>(var->data.get()[ii]);
         }
       } else {  // Required record is not found
         std::string ErrorMsg =
                "DataContainer::inquire: " + variable + " : " + group +" is not found";
         ABORT(ErrorMsg);
       }
     }

     // -----------------------------------------------------------------------------

     /*! \brief Insert/Update the vector of Record to container*/
     template <typename Type>
     void insert(const std::string & group, const std::string & variable,
                  const std::size_t vsize, const Type vdata[]) {
       if (has(group, variable)) {  // Found the required record in database
         auto var = DataContainer.find(boost::make_tuple(group, variable));

         // Check if there is "w" or "W" in the read & write mode
         std::string mode = var->mode;
         std::transform(mode.begin(), mode.end(), mode.begin(), ::tolower);
         std::size_t found = mode.find("w");
         if (found == std::string::npos) {  // It is read-only
            std::string ErrorMsg =
                "DataContainer::Insert/Update: trying to overwrite an read-only record : "
                + variable + " : " + group;
            ABORT(ErrorMsg);
         }

         // Update the record
         for (std::size_t ii = 0; ii < vsize; ++ii)
           var->data.get()[ii] = vdata[ii];

       } else {  // The required record in not in database, update the database
         std::unique_ptr<boost::any[]> vect{ new boost::any[vsize] };

         for (std::size_t ii = 0; ii < vsize; ++ii)
           vect.get()[ii] = static_cast<Type>(vdata[ii]);

         DataContainer.insert({group, variable, vsize, vect});
       }
     }

     // -----------------------------------------------------------------------------

 private:
     /*! \brief container instance */
     Record_set DataContainer;

     /*! \brief number of locations on this PE */
     std::size_t nlocs_;

     /*! \brief number of observational variables */
     std::size_t nvars_;

     /*! \brief Print */
     void print(std::ostream &) const;
};

}  // namespace ioda

#endif  // DATABASE_MULTIINDEXCONTAINER_H_
