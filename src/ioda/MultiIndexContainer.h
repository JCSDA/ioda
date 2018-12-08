/*
 * (C) Copyright 2017 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef IODA_MULTIINDEXCONTAINER_H_
#define IODA_MULTIINDEXCONTAINER_H_

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

#include "fileio/IodaIO.h"
#include "fileio/IodaIOfactory.h"

#include "oops/util/abor1_cpp.h"
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

     struct by_group {};
     struct by_name {};
     struct Record {
         std::string group; /*!< Group name: such as ObsValue, HofX, MetaData, ObsErr etc. */
         std::string name;  /*!< Variable name */
         std::size_t size;  /*!< Array size */
         std::unique_ptr<boost::any[]> data; /*!< Smart pointer to array */

         // Constructors
         Record(
          const std::string & group_, const std::string & name_, const std::size_t & size_,
                std::unique_ptr<boost::any[]> & data_):
          group(group_), name(name_), size(size_)
          {
            data.reset(new boost::any[size]);
            for (std::size_t ii = 0; ii < size; ++ii)
              data.get()[ii] = data_.get()[ii];
          }

         // -----------------------------------------------------------------------------

         friend std::ostream& operator<<(std::ostream & os, const Record & v) {
             os << v.group << ", " << v.name << ": { ";
             for (std::size_t i=0; i != std::min(v.size, static_cast<std::size_t>(10)); ++i) {
                if (v.data[i].type() == typeid(int)) {
                    os << boost::any_cast<int>(v.data[i]) << " ";
                } else if (v.data[i].type() == typeid(float)) {
                    os << boost::any_cast<float>(v.data[i]) << " ";
                } else if (v.data[i].type() == typeid(double)) {
                    os << boost::any_cast<double>(v.data[i]) << " ";
                } else if (v.data[i].type() == typeid(std::string)) {
                    os << boost::any_cast<std::string>(v.data[i]) << " ";
                } else {
                    os << "iostream is not implmented yet " << __LINE__  << __FILE__ << " ";
                }
             }
             os << "}";
             return os;
         }
     };

     // -----------------------------------------------------------------------------

     using Record_set = multi_index_container<
         Record,
         indexed_by<
             ordered_unique<
                composite_key<
                    Record,
                    member<Record, std::string, &Record::group>,
                    member<Record, std::string, &Record::name>
                >
             >,
             // non-unique as there are many Records under group
             ordered_non_unique<
                tag<by_group>,
                member<
                    Record, std::string, &Record::group
                >
             >,
             // non-unique as there are Records with the same name in different group
             ordered_non_unique<
                tag<by_name>,
                member<
                    Record, std::string, &Record::name
                >
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

     /*! \brief Check the availability of Record in container*/
     bool has(const std::string & group, const std::string & name) const;

     /*! \brief Return the number of locations on this PE*/
     std::size_t nlocs() const {return nlocs_;}

     /*! \brief Return the number of observational variables*/
     std::size_t nvars() const {return nvars_;}

     // -----------------------------------------------------------------------------

     /*! \brief Inquire the vector of Record from container*/
     template <typename Type>
     void get_var(const std::string & group, const std::string & name,
                  const std::size_t vsize, Type vdata[]) const {
       if (has(group, name)) {
         auto var = DataContainer.find(boost::make_tuple(group, name));
         const std::type_info & typeInput = var->data.get()->type();
         const std::type_info & typeOutput = typeid(Type);
         if ((typeInput == typeid(float)) && (typeOutput == typeid(double))) {
           for (std::size_t ii = 0; ii < vsize; ++ii)
             vdata[ii] = static_cast<Type>(boost::any_cast<float>(var->data.get()[ii]));
         } else {
           if (typeInput != typeOutput)
             oops::Log::warning() << "DataContainer::get_var: inconsistent type :"
                                  << " name@group: " << name << "@" << group << " "
                                  << std::string(typeInput.name()) + " @ container vs. "
                                  << std::string(typeOutput.name()) + " @ output" << std::endl;
           for (std::size_t ii = 0; ii < vsize; ++ii)
             vdata[ii] = boost::any_cast<Type>(var->data.get()[ii]);
         }
       } else {
         std::string ErrorMsg = "DataContainer::get_var: " + name + "@" + group +" is not found";
         ABORT(ErrorMsg);
       }
     }

     // -----------------------------------------------------------------------------

     /*! \brief Insert/Update the vector of Record to container*/
     template <typename Type>
     void put_var(const std::string & group, const std::string & name,
                  const std::size_t vsize, const Type vdata[]) {
       if (has(group, name)) {
         auto var = DataContainer.find(boost::make_tuple(group, name));
         oops::Log::debug() << *var << std::endl;
         for (std::size_t ii = 0; ii < vsize; ++ii)
           var->data.get()[ii] = vdata[ii];
       } else {
         std::unique_ptr<boost::any[]> vect{ new boost::any[vsize] };
         for (std::size_t ii = 0; ii < vsize; ++ii)
           vect.get()[ii] = static_cast<Type>(vdata[ii]);
         DataContainer.insert({group, name, vsize, vect});
       }
     }

     // -----------------------------------------------------------------------------

 private:
     /*! \brief container instance */
     Record_set DataContainer;

     /*! \brief file IO object of input */
     std::unique_ptr<ioda::IodaIO> fileio_;

     /*! \brief number of locations on this PE */
     std::size_t nlocs_;

     /*! \brief number of observational variables */
     std::size_t nvars_;

     /*! \brief read the vector of Record from file*/
     void read_var(const std::string & group, const std::string & name);

     /*! \brief Print */
     void print(std::ostream &) const;
};

}  // namespace ioda

#endif  // IODA_MULTIINDEXCONTAINER_H_
