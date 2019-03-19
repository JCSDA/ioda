/*
 * (C) Copyright 2017 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef DATABASE_OBSSPACECONTAINER_H_
#define DATABASE_OBSSPACECONTAINER_H_

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <tuple>
#include <typeinfo>
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
#include "oops/util/missingValues.h"
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
 private:
     // --------------------------------------------------------------------------------
     // Following is the definition of multi index structure used for observation
     // data storage. There are three sections.
     //
     // The first declares dummy structs that are used to give names to the multi index
     // indexing methods (third section). Without these you need to use numbers and then
     // the order you declare the indexing methods becomes tied to these numbers.
     //
     // The second section declares the struct that will be the element struct that the
     // multi index structure manages. The struct elements that are the keys for the
     // multi index structure are declared first by convention.
     //
     // The third section declares the multi index structure which manages the set of
     // elements defined in the second section. The multi index structure is where the
     // manner of access and organization of the set of elements is defined.

     // Section 1: dummy structs for naming indexing methods
     struct by_group {};              // Index by group name
     struct by_variable {};           // Index by variable name

     // Section 2: element struct managed by the multi index structure
     struct VarRecord {
         // Keys
         std::string group;           /*!< Group name: ObsValue, HofX, MetaData, ObsErr etc. */
         std::string variable;        /*!< Variable name */

         // Attributes
         std::string mode;                  /*!< Read & write mode : 'r' or 'rw'*/
         const std::type_info & type;       /*!< Type of data */
         std::size_t size;                  /*!< Total size of data */
         std::vector<std::size_t> shape;    /*!< Shape of data */

         // Data
         std::unique_ptr<boost::any[]> data;  /*!< Smart pointer to vector */

         // Constructor with default read & write mode : "rw"
         VarRecord(const std::string & group, const std::string & variable,
                const std::type_info & type,
                const std::vector<std::size_t> & shape, const std::size_t & size,
                std::unique_ptr<boost::any[]> & vect)
            : group(group), variable(variable), mode("rw"), type(type),
              shape(shape), size(size), data(std::move(vect)) {}

         // Constructor with passed read & write mode
         VarRecord(const std::string & group, const std::string & variable,
                   const std::string & mode, const std::type_info & type,
                   const std::vector<std::size_t> & shape,
                   const std::size_t & size, std::unique_ptr<boost::any[]> & vect)
            : group(group), variable(variable), mode(mode), type(type),
              shape(shape), size(size), data(std::move(vect)) {}
     };  // end of VarRecord definition

     // Section 3: indexing methods, organization of elements
     using VarRecord_set = multi_index_container<
         VarRecord,
         indexed_by<
             ordered_unique<
                composite_key<
                    VarRecord,
                    member<VarRecord, std::string, &VarRecord::group>,
                    member<VarRecord, std::string, &VarRecord::variable>
                >
             >,
             // non-unique as there are many VarRecords under group
             ordered_non_unique<
                tag<by_group>,
                member<VarRecord, std::string, &VarRecord::group>
             >,
             // non-unique as there are VarRecords with the same name in different group
             ordered_non_unique<
                tag<by_variable>,
                member<VarRecord, std::string, &VarRecord::variable>
             >
         >
     >;

 public:
     ObsSpaceContainer();
     ~ObsSpaceContainer();

     // Access to iterators
     typedef VarRecord_set::index<ObsSpaceContainer::by_variable>::type VarIndex;
     typedef VarIndex::iterator VarIter;

     /*! \brief Variables iterator begin */
     VarIter var_iter_begin();

     /*! \brief Variables iterator end */
     VarIter var_iter_end();

     /*! \brief Variables iterator variable name */
     std::string var_iter_vname(VarIter);

     /*! \brief Variables iterator group name */
     std::string var_iter_gname(VarIter);

     /*! \brief Variables iterator mode */
     std::string var_iter_mode(VarIter);

     /*! \brief Variables iterator type */
     const std::type_info & var_iter_type(VarIter);

     /*! \brief Variables iterator size */
     std::size_t var_iter_size(VarIter);

     /*! \brief Variables iterator shape */
     std::vector<std::size_t> var_iter_shape(VarIter);

     /*! \brief Check the availability of VarRecord with group and variable in container*/
     bool has(const std::string & group, const std::string & variable) const;

     /*! \brief Return the number of uniqure observation locations on this PE*/
     std::size_t nlocs() const {return nlocs_;}

     /*! \brief Return the number of observational variables*/
     std::size_t nvars() const {return nvars_;}

     // -----------------------------------------------------------------------------

     /*! \brief Load VarData from the container*/
     void LoadFromDb(const std::string & GroupName, const std::string & VarName,
                  const std::vector<std::size_t> & VarShape, int * VarData) const;
     void LoadFromDb(const std::string & GroupName, const std::string & VarName,
                  const std::vector<std::size_t> & VarShape, float * VarData) const;
     void LoadFromDb(const std::string & GroupName, const std::string & VarName,
                  const std::vector<std::size_t> & VarShape, std::string * VarData) const;
     void LoadFromDb(const std::string & GroupName, const std::string & VarName,
                  const std::vector<std::size_t> & VarShape, util::DateTime * VarData) const;

     // -----------------------------------------------------------------------------

     /*! \brief Store VarData into the container*/
     void StoreToDb(const std::string & GroupName, const std::string & VarName,
                    const std::vector<std::size_t> & VarShape, const int * VarData);
     void StoreToDb(const std::string & GroupName, const std::string & VarName,
                    const std::vector<std::size_t> & VarShape, const float * VarData);
     void StoreToDb(const std::string & GroupName, const std::string & VarName,
                    const std::vector<std::size_t> & VarShape, const std::string * VarData);
     void StoreToDb(const std::string & GroupName, const std::string & VarName,
                    const std::vector<std::size_t> & VarShape, const util::DateTime * VarData);

 private:
     /*! \brief helper function for LoadFromDb */
     template <typename DataType>
     void LoadFromDb_helper(const std::string & GroupName, const std::string & VarName,
                      const std::vector<std::size_t> & VarShape, DataType * VarData) const;

     /*! \brief helper function for StoreToDb */
     template <typename DataType>
     void StoreToDb_helper(const std::string & GroupName, const std::string & VarName,
                      const std::vector<std::size_t> & VarShape, const DataType * VarData);

     /*! \brief container instance */
     VarRecord_set DataContainer;

     /*! \brief number of locations on this PE */
     std::size_t nlocs_;

     /*! \brief number of observational variables */
     std::size_t nvars_;

     /*! \brief Print */
     void print(std::ostream &) const;
};

}  // namespace ioda

#endif  // DATABASE_OBSSPACECONTAINER_H_
