/*
 * (C) Copyright 2017 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */

#ifndef DATABASE_OBSSPACECONTAINER_H_
#define DATABASE_OBSSPACECONTAINER_H_

#define BOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED

#include <algorithm>
#include <functional>
#include <iostream>
#include <memory>
#include <numeric>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

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

#include "utils/IodaUtils.h"

using boost::multi_index::composite_key;
using boost::multi_index::indexed_by;
using boost::multi_index::member;
using boost::multi_index::multi_index_container;
using boost::multi_index::ordered_non_unique;
using boost::multi_index::ordered_unique;
using boost::multi_index::tag;

namespace ioda {
/*!
 * \brief Obs container class for IODA
 *
 * \details This class provides a container to hold obs data in memory for use by the
 *          ObsSpace class. The container is a Boost Multi Index container (header-only
 *          implementation from Boost) which essentially creates indexing structures 
 *          that point to a set of C structs. Each structure holds the keys that identify
 *          that structure and the data the are associated with those keys. There are two
 *          keys, group and variable, which identify each struct. The group corresponds
 *          to collections of variables such as "ObsValue", "ObsError", "PreQC" and
 *          "MetaData". The variables are individual quantities such as "air_temperature"
 *          and "brightness_temperature".
 *
 * \author Xin Zhang (JCSDA)
 */
template <typename ContType>
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
     /*!
      * \brief elemental struct of the obs container
      *
      * \details This struct represents a single entry in the obs container. It contains
      *          both the keys that identify the entry and the data held in this entry.
      */
     struct VarRecord {
         // Keys
         std::string group;     /*!< Group name: ObsValue, HofX, MetaData, ObsErr etc. */
         std::string variable;  /*!< Variable name */

         // Attributes
         std::string mode;                /*!< Read & write mode : 'r' or 'rw'*/
         std::vector<std::size_t> shape;  /*!< Shape of data */
             /*!< Note that shape holds the dimension sizes and size is the product */
             /*!< of these dimension sizes. */

         // Data
         std::vector<ContType> data;

         // Constructor with default read & write mode : "rw"
         VarRecord(const std::string & group, const std::string & variable,
                const std::vector<std::size_t> & shape, std::vector<ContType> & vect)
            : group(group), variable(variable), mode("rw"),
              shape(shape), data(std::move(vect)) {}

         // Constructor with passed read & write mode
         VarRecord(const std::string & group, const std::string & variable,
                   const std::string & mode, const std::vector<std::size_t> & shape,
                   std::vector<ContType> & vect)
            : group(group), variable(variable), mode(mode),
              shape(shape), data(std::move(vect)) {}
     };  // end of VarRecord definition

     // Section 3: indexing methods, organization of elements
     /*!
      * \brief Multi Index container for obs data
      *
      * \details This typedef defines the indexing structure of the obs data container.
      *          The primary index is a composite key (tuple) consisting of the group
      *          and variable names. The iterator of the container itself uses this
      *          primary index, as well as the typical container methods such as find
      *          and insert. Two more secondary indexes are provided, one by group and the
      *          other by variable. These secondary indexes have their own iterators
      *          associated with them. See the boost::multiindex documentation for more
      *          details.
      */
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
     /*!
      * \brief index by variable
      *
      * \details This typedef defines the index mechanism that allows access to the
      *          obs container through the secondary indexing by variable.
      */
     typedef typename
       boost::multi_index::index<VarRecord_set, by_variable>::type VarIndex;

     /*!
      * \brief variable iterator
      *
      * \details This typedef defines an iterator that can walk through the obs container
      *          by variable. It utilizes the secondary indexing by variable.
      */
     typedef typename VarIndex::iterator VarIter;

     /*!
      * \brief container iterator
      *
      * \details This typedef defines an iterator that can walk through the obs container
      *          using the primary indexing (by group and variable). 
      */
     typedef typename VarRecord_set::iterator DbIter;

     VarIter var_iter_begin();
     VarIter var_iter_end();

     std::string var_iter_vname(VarIter);
     std::string var_iter_gname(VarIter);
     std::string var_iter_mode(VarIter);
     std::size_t var_iter_size(VarIter);
     std::vector<std::size_t> var_iter_shape(VarIter);

     DbIter find(const std::string &, const std::string &) const;
     DbIter begin() const;
     DbIter end() const;

     const std::type_info & dtype(const DbIter) const;
     const std::type_info & dtype(const std::string &, const std::string &) const;

     bool has(const std::string & group, const std::string & variable) const;

     /*! \brief Return the number of uniqure observation locations on this PE*/
     std::size_t nlocs() const {return nlocs_;}

     /*! \brief Return the number of observational variables*/
     std::size_t nvars() const {return nvars_;}

     void LoadFromDb(const std::string & GroupName, const std::string & VarName,
                     const std::vector<std::size_t> & VarShape, std::vector<ContType> & VarData,
                     const std::size_t Start = 0, const std::size_t Count = 0) const;

     void StoreToDb(const std::string & GroupName, const std::string & VarName,
                    const std::vector<std::size_t> & VarShape,
                    const std::vector<ContType> & VarData,
                    const bool Append = false);

 private:
     /*! \brief set end of vector segment */
     static std::size_t SetSegmentEnd(std::size_t Start, std::size_t Count,
                                      std::size_t VarSize);

     /*! \brief obs container instance */
     VarRecord_set DataContainer;

     /*! \brief number of locations on this PE */
     std::size_t nlocs_;

     /*! \brief number of observational variables */
     std::size_t nvars_;

     /*! \brief Print */
     void print(std::ostream &) const;
};

// -----------------------------------------------------------------------------

template <typename ContType>
ObsSpaceContainer<ContType>::ObsSpaceContainer() {
  oops::Log::trace() << "ioda::ObsSpaceContainer Constructor starts " << std::endl;
}

// -----------------------------------------------------------------------------

template <typename ContType>
ObsSpaceContainer<ContType>::~ObsSpaceContainer() {
  oops::Log::trace() << "ioda::ObsSpaceContainer deconstructed " << std::endl;
}

// -----------------------------------------------------------------------------
/*!
 * \brief store data into the obs container
 *
 * \details This method transfers data from the caller's memory into the obs conatiner.
 *          The caller needs to allocate and assign the memory that the VarData parameter
 *          points to. This method also handles checking that the varible has write
 *          permission if it already exists in the obs container.
 *
 * \param[in] GroupName Name of container group (ObsValue, ObsError, MetaData, etc.)
 * \param[in] VarName Name of container variable
 * \param[in] VarShape Dimension sizes of variable
 * \param[in] VarData Pointer to memory that will be stored in the container
 */

template <typename ContType>
void ObsSpaceContainer<ContType>::StoreToDb(const std::string & GroupName,
          const std::string & VarName, const std::vector<std::size_t> & VarShape,
          const std::vector<ContType> & VarData, bool Append) {
  // Search for VarRecord with GroupName, VarName combination
  DbIter Var = DataContainer.find(boost::make_tuple(GroupName, VarName));
  if (Var != DataContainer.end()) {
    // Found the required record in database
    // Check if attempting to update a read only VarRecord
    if (Var->mode.compare("r") == 0) {
      std::string ErrorMsg =
                  "ObsSpaceContainer::StoreToDb: trying to overwrite a read-only record : "
                  + VarName + " : " + GroupName;
      ABORT(ErrorMsg);
    }

    // Update the record (boost replace method)
    VarRecord DbRec = *Var;
    if (Append) {
      DbRec.data.insert(DbRec.data.end(), VarData.begin(), VarData.end());
    } else {
      DbRec.data = VarData;
    }
    DbRec.shape[0] = DbRec.data.size();
    DataContainer.replace(Var, DbRec);
  } else {
    // The required record is not in database, update the database
    std::vector<ContType> vect = VarData;
    VarRecord DbRec(GroupName, VarName, VarShape, vect);
    DataContainer.insert(DbRec);
  }
}

// -----------------------------------------------------------------------------
/*!
 * \brief load data from the obs container
 *
 * \details This method transfers data from the obs container to the caller's memory.
 *          The caller needs to allocate the memory that the VarData parameter points to.
 *          This method handles the lookup of the container group, variable entry, and
 *          checking that the data type in the container matches that of the VarData
 *          parameter.
 *
 * \param[in] GroupName Name of container group (ObsValue, ObsError, MetaData, etc.)
 * \param[in] VarName Name of container variable
 * \param[in] VarShape Dimension sizes of variable
 * \param[in] VarData Pointer to memory that will loaded from the container.
 */

template <typename ContType>
void ObsSpaceContainer<ContType>::LoadFromDb(const std::string & GroupName,
              const std::string & VarName, const std::vector<std::size_t> & VarShape,
              std::vector<ContType> & VarData, std::size_t Start, std::size_t Count) const {
  if (has(GroupName, VarName)) {
    // Found the required record in the database
    DbIter Var = DataContainer.find(boost::make_tuple(GroupName, VarName));

    // Calculate the total number of elements
    std::size_t VarSize =
      std::accumulate(VarShape.begin(), VarShape.end(), 1, std::multiplies<std::size_t>());

    // Set the start and end of the vector segment
    std::size_t End = SetSegmentEnd(Start, Count, VarSize);

    // Copy the elements into the output
    for (std::size_t i = Start; i < End; i++) {
      VarData[i-Start] = Var->data[i];
    }
  } else {
    // Required record is not in the database
    std::string ErrorMsg =
           "ObsSpaceContainer::LoadFromDb: " + VarName + " @ " + GroupName +" is not found";
    ABORT(ErrorMsg);
  }
}

// -----------------------------------------------------------------------------
/*!
 *
 * \details This method returns the ending index for the vector segment given
 *          the start and count values.
 *
 */
template <typename ContType>
std::size_t ObsSpaceContainer<ContType>::SetSegmentEnd(std::size_t Start, std::size_t Count,
                                                       std::size_t VarSize) {
  std::size_t End;
  if (Count > 0) {
    End = Start + Count;
    if (End > VarSize) {
      std::string ErrorMsg =
                  "ObsSpaceContainer::SetSegmentEnd: Start plus Count goes past end of vector: "
                  + std::to_string(Start) + " + " + std::to_string(Count) + " > "
                  + std::to_string(VarSize);
      ABORT(ErrorMsg);
    }
  } else {
    End = VarSize;
  }
  return End;
}

// -----------------------------------------------------------------------------
/*!
 *
 * \details This method returns the begin iterator for access by variable in the
 *          obs container.
 *
 */
template <typename ContType>
typename ObsSpaceContainer<ContType>::VarIter ObsSpaceContainer<ContType>::var_iter_begin() {
  VarIndex & var_index_ = DataContainer.template get<by_variable>();
  return var_index_.begin();
}

// -----------------------------------------------------------------------------
/*!
 *
 * \details This method returns the end iterator for access by variable in the
 *          obs container.
 *
 */
template <typename ContType>
typename ObsSpaceContainer<ContType>::VarIter ObsSpaceContainer<ContType>::var_iter_end() {
  VarIndex & var_index_ = DataContainer.template get<by_variable>();
  return var_index_.end();
}

// -----------------------------------------------------------------------------
/*!
 *
 * \details This method returns an obs container iterator that indicates if the
 *          given group, variable entry exists. If the entry exists, then the
 *          iterator value that is returned will point to that entry. Otherwise,
 *          the iterator is set to the end() value to indicate the entry does
 *          is not found.
 *
 * \param[in] group Name of obs container group
 * \param[in] variable Name of obs container variable
 *
 */
template <typename ContType>
typename ObsSpaceContainer<ContType>::DbIter ObsSpaceContainer<ContType>::find(
                       const std::string & group, const std::string & variable) const {
    DbIter var = DataContainer.find(boost::make_tuple(group, variable));
    return var;
}

// -----------------------------------------------------------------------------
/*!
 *
 * \details This method returns the begin iterator for the obs container.
 *
 */
template <typename ContType>
typename ObsSpaceContainer<ContType>::DbIter ObsSpaceContainer<ContType>::begin() const {
    return DataContainer.begin();
}

// -----------------------------------------------------------------------------
/*!
 *
 * \details This method returns the end iterator for the obs container.
 *
 */
template <typename ContType>
typename ObsSpaceContainer<ContType>::DbIter ObsSpaceContainer<ContType>::end() const {
    return DataContainer.end();
}

// -----------------------------------------------------------------------------
/*!
 *
 * \details This method returns the data type associated with the obs container
 *          entry pointed to by Idb.
 *
 * \param[in] Idb Iterator pointing to the current element of the obs container.
 */
template <typename ContType>
const std::type_info & ObsSpaceContainer<ContType>::dtype(const DbIter Idb) const {
    return Idb->type;
}

// -----------------------------------------------------------------------------
/*!
 *
 * \details This method returns the data type associated with the obs container
 *          entry defined by the given group and variable. If the entry does not
 *          exist, the typeid for void is returned.
 *
 * \param[in] group Name of obs container group
 * \param[in] variable Name of obs container variable
 */
template <typename ContType>
const std::type_info & ObsSpaceContainer<ContType>::dtype(const std::string & group,
                                                const std::string & variable) const {
    DbIter Var = DataContainer.find(boost::make_tuple(group, variable));
    if (Var == DataContainer.end()) {
      return typeid(void);
    } else {
      return Var->type;
    }
}

// -----------------------------------------------------------------------------
/*!
 *
 * \details This method returns the variable name associated with the obs container
 *          entry pointed to by var_iter.
 *
 * \param[in] var_iter Iterator of the indexing by variable type.
 */
template <typename ContType>
std::string ObsSpaceContainer<ContType>::var_iter_vname(
                                         ObsSpaceContainer<ContType>::VarIter var_iter) {
  return var_iter->variable;
}

// -----------------------------------------------------------------------------
/*!
 *
 * \details This method returns the group name associated with the obs container
 *          entry pointed to by var_iter.
 *
 * \param[in] var_iter Iterator of the indexing by variable type.
 */
template <typename ContType>
std::string ObsSpaceContainer<ContType>::var_iter_gname(
                                         ObsSpaceContainer<ContType>::VarIter var_iter) {
  return var_iter->group;
}

// -----------------------------------------------------------------------------
/*!
 *
 * \details This method returns the access mode associated with the obs container
 *          entry pointed to by var_iter.
 *
 * \param[in] var_iter Iterator of the indexing by variable type.
 */
template <typename ContType>
std::string ObsSpaceContainer<ContType>::var_iter_mode(
                                         ObsSpaceContainer<ContType>::VarIter var_iter) {
  return var_iter->mode;
}

// -----------------------------------------------------------------------------
/*!
 *
 * \details This method returns the data size associated with the obs container
 *          entry pointed to by var_iter.
 *
 * \param[in] var_iter Iterator of the indexing by variable type.
 */
template <typename ContType>
std::size_t ObsSpaceContainer<ContType>::var_iter_size(
                                         ObsSpaceContainer<ContType>::VarIter var_iter) {
  return var_iter->data.size();
}

// -----------------------------------------------------------------------------
/*!
 *
 * \details This method returns the data shape associated with the obs container
 *          entry pointed to by var_iter.
 *
 * \param[in] var_iter Iterator of the indexing by variable type.
 */
template <typename ContType>
std::vector<std::size_t> ObsSpaceContainer<ContType>::var_iter_shape(
                                           ObsSpaceContainer<ContType>::VarIter var_iter) {
  return var_iter->shape;
}

// -----------------------------------------------------------------------------
/*!
 *
 * \details This method returns a boolean that indicates if the given group, variable
 *          entry exists in the obs container. If the entry exists, then "true" is
 *          returned. Otherwise, "false" is returned.
 *
 * \param[in] group Name of obs container group
 * \param[in] variable Name of obs container variable
 *
 */
template <typename ContType>
bool ObsSpaceContainer<ContType>::has(const std::string & group,
                                      const std::string & variable) const {
  DbIter var = find(group, variable);
  return (var != DataContainer.end());
}

// -----------------------------------------------------------------------------
/*!
 * \brief print method for Printable base class
 *
 * \details This method provides a print routine so that the obs container can be
 *          used in an output stream. A list of all group, variable combinations
 *          present in the obs container is printed out.
 */
template <typename ContType>
void ObsSpaceContainer<ContType>::print(std::ostream & os) const {
  const VarIndex & var = DataContainer.template get<by_variable>();
  os << "ObsSpace Multi.Index Container for IODA" << "\n";
  for (VarIter iter = var.begin(); iter != var.end(); ++iter)
    os << iter->variable << " @ " << iter->group << "\n";
}

// -----------------------------------------------------------------------------

}  // namespace ioda

#endif  // DATABASE_OBSSPACECONTAINER_H_
