/*
 * (C) Copyright 2021 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
/*!
 * \defgroup ioda_cxx_ex_adv Advanced Examples
 * \brief Advanced usage examples
 * \ingroup ioda_cxx_ex
 * 
 * \defgroup ioda_cxx_ex_types Using the Type System
 * \brief Examples of using more advanced features of the type
 *   system. Array types, fixed-length strings, enumerations, etc.
 * \ingroup ioda_cxx_ex_adv
 *
 * @{
 *
 * \defgroup ioda_cxx_ex_types_array_from_struct Array data types (1)
 * \brief Writing a complex structure or class using array data types.
 * 
 * @{
 * 
 * \file array_from_struct.cpp
 * \details
 * 
 * IODA provides a rich data type system that allows users to write simple data
 * types (integers, floats, bools), strings (of either fixed or variable lengths),
 * arrays (fixed or variable length), enumerations (categorical data with meanings
 * associated with each number), and compound objects (mixtures of everything else).
 * 
 * This example shows how you can customize IODA to automatically understand and write
 * data associated with a new class. The "Something_Like_DateTime" struct is a
 * simplified version of the DateTime class in JEDI's OOPS library (oops::DateTime).
 * It has two data members, date and time, that always are read and written together.
 * By specializing a few template definitions, IODA can learn about how the struct
 * is organized and how it should be written and read.
 * 
 * This example can be divided into a few logical parts:
 * - The Something_Like_DateTime definition,
 * - FillValuePolicies (default values for missing or unwritten data),
 * - GetType (creating a new IODA type),
 * - Object Accessor (read/write; serialization & deserialization to/from byte streams), and the
 * - main function.
 * 
 * The class definition, Fill value defaults, GetType, and Object Accessor need to be
 * implemented once per class. They can all exist within the same header file, just like with
 * this example, although you really should write the declarations in a header and add the
 * function definitions into a separate source code file.
 **/

#include <iostream>
#include <vector>

#include "ioda/Engines/Factory.h"
#include "ioda/Exception.h"
#include "ioda/Group.h"

/// \brief An example class that we would like to read and write using IODA.
struct Something_Like_DateTime {
  uint64_t date;  ///< The date. Expressed as YYYYMMDD.
  uint64_t time;  ///< The time in UTC. Expressed as HHMMSS.

  bool operator==(const Something_Like_DateTime& rhs) const {
    if (rhs.date != date) return false;
    if (rhs.time != time) return false;
    return true;
  }
  bool operator!=(const Something_Like_DateTime& rhs) const { return !operator==(rhs); }
};

namespace ioda {

/// \brief Specify default fill values that are used when creating new
///   Something_Like_DateTime objects.
/// \see FillPolicy.h for where these defaults are normally defined.
template <>
inline Something_Like_DateTime FillValuePolicies::FillValue_default<Something_Like_DateTime>() {
  return Something_Like_DateTime{0, 0};
}

/// \brief Instructs IODA how to create a Data Type that can handle Something_Like_DateTime data.
/// \param t is the IODA backend's Type_Provider interface. This interface exposes the backend functions to
///   make basic data types (ints, floats, doubles, chars), array types (int[2]; fixed and variable length),
///   string types (variable-length, fixed-length), enumerated types, and compound object types (packet tables).
///   Complex objects are built up from simpler data types.
/// \note
///   When specializing, only one function parameter is needed (t). The other parameters are used by different
///   signatures of the full template.
/// \details
///   Something_Like_DateTime is represented as a pair of unsigned 64-bit integers. In this case, GetType
///   is really a one-liner.
/// \see Object_Accessor_Something_Like_Datetime for an implementation of reading / writing
///   Something_Like_DateTime objects.
/// \see Type.h for where these overrides are normally defined.
template <>
Type Types::GetType<Something_Like_DateTime, 0>(
  gsl::not_null<const ::ioda::detail::Type_Provider*> t, std::initializer_list<Dimensions_t>,
  void*) {
  return t->makeArrayType({2}, typeid(Something_Like_DateTime), typeid(uint64_t));
}

namespace detail {

/// \brief Binding code to allow reads and writes directly to Something_Like_DateTime objects.
///   Basically teaches IODA about how Something_Like_DateTime objects are structured.
/// \see Marshalling.h. All of the generic templates are implemented there.
/// \details
///   When you write an object using IODA, IODA needs to know *what precisely* to write. If you are writing a
///   struct or a class, what data members are being written? How are they packed together? Are
///   the fields adjacent, or are there alignment issues? What are the types of the data? Et cetera.
///   The Object Accessor is a special struct that uses this information to turn objects into byte streams.
///   The Object Accessor also works in the reverse direction when reading to turn byte streams back into objects.
///
///   There are three main functions. Two for reading, and one for writing.
///   - serialize helps write an object
///   - prep_deserialize and deserialize help read an object.
///
///   When attributes and variables read and write, these functions get called immediately
///   before / after control passes between the frontend part of IODA (the header files) and
///   the engine implementations (ObsStore, HDF5, etc.). To
///   see how they work, consult Attribute_Base::read(gsl::span<DataType>) and
///   Attribute_Base::write(gsl::span<const DataType> data).
///
///   Even for a simple object (two unsigned ints) you always need to do serialization and
///   deserialization instead of direct memory access because the compiler can align
///   class members and pad memory. There might be some empty space between data members!
struct Object_Accessor_Something_Like_DateTime {
  /// The serialized_type is a container for a byte stream of data from Something_Like_DateTime. Used for writing.
  typedef std::shared_ptr<Marshalled_Data<uint64_t>> serialized_type;
  /// Similar to serialized_type, but used when reading.
  typedef std::shared_ptr<const Marshalled_Data<uint64_t>> const_serialized_type;
  /// Who owns the data pointers? Is this data produced by a backend (usually from a read operation) or is it
  /// coming from the caller (usually when writing)? This tells us when the data can be freed.
  detail::PointerOwner pointerOwner_;
  /// How big is each object?
  static constexpr size_t bytesPerObject_ = 2;  // Each object has a date and a time.

  Object_Accessor_Something_Like_DateTime(detail::PointerOwner pointerOwner
                                          = detail::PointerOwner::Caller)
      : pointerOwner_(pointerOwner) {}
  /// \brief Converts an object into a byte stream.
  /// \note const_serialized_type is self-destructing and takes care of deallocation when we no longer
  ///   need the serialized buffer.
  /// \see Marshalled_Data in Marshalling.h.
  const_serialized_type serialize(::gsl::span<const Something_Like_DateTime> d) {
    // Marshalled_Data implements a vector of values. These values are numbers for fundamental numeric types,
    // but are typically pointers for more complex types.
    auto res          = std::make_shared<Marshalled_Data<uint64_t>>();
    res->DataPointers = std::vector<uint64_t>(d.size() * bytesPerObject_);

    for (size_t i = 0; i < (size_t)d.size(); ++i) {
      res->DataPointers[2 * i + 0] = d[i].date;
      res->DataPointers[2 * i + 1] = d[i].time;
    }

    return res;
  }

  /// \brief Helper function when creating objects from byte streams. We know how many objects we want to
  ///   create, and this function allocates a buffer large enough to contain the data.
  /// \param numObjects is the number of objects that are read.
  /// \details In the case of Something_Like_Datetime objects, we expect each object to be made up of
  ///   two uint64_t data fields (date and time).
  serialized_type prep_deserialize(size_t numObjects) {
    auto res          = std::make_shared<Marshalled_Data<uint64_t>>(pointerOwner_);
    res->DataPointers = std::vector<uint64_t>(numObjects * bytesPerObject_);
    return res;
  }

  /// \brief Function that converts the data from a stream into distinct objects. These objects
  ///   already exist, thanks to prep_deserialize. The deserialize function fills in data members.
  void deserialize(serialized_type p, gsl::span<Something_Like_DateTime> data) {
    const size_t ds = data.size(), dp = p->DataPointers.size();
    if (ds != dp / bytesPerObject_)
      throw Exception("You are reading the wrong amount of data!", ioda_Here())
        .add("data.size()", ds)
        .add("p->DataPointers.size()", dp);

    for (size_t i = 0; i < (size_t)data.size(); ++i) {
      data[i].date = p->DataPointers[2 * i + 0];
      data[i].time = p->DataPointers[2 * i + 1];
    }
  }
};

template <>
struct Object_AccessorTypedef<Something_Like_DateTime> {
  typedef Object_Accessor_Something_Like_DateTime type;
};
}  // end namespace detail
}  // end namespace ioda

/// \brief The main program. Reads and writes data.
int main(int argc, char** argv) {
  using namespace ioda;
  using namespace std;
  try {
    // Use the HDF5 file backend by default.
    auto f = Engines::constructFromCmdLine(argc, argv, "arrays_from_struct.hdf5");

    // We write and read data using both attributes and variables.

    // Some data
    const std::vector<Something_Like_DateTime> datetimes{
      {20210101, 0}, {20210101, 120000}, {20210913, 143000}};

    {  // Write an attribute
      const struct Something_Like_DateTime start { 20210913, 101100 };
      const struct Something_Like_DateTime end { 20210913, 161100 };
      f.atts.create<Something_Like_DateTime>("Start_Date").write<Something_Like_DateTime>(start);
      f.atts.create<Something_Like_DateTime>("End_Date").write<Something_Like_DateTime>(end);

      f.atts.create<Something_Like_DateTime>("dates", {3})
        .write<Something_Like_DateTime>(datetimes);
    }

    {  // Querying an array data type
      Type typ = f.atts["dates"].getType();

      // Check that this is an array type.
      if (typ.getClass() != TypeClass::FixedArray) throw Exception("Wrong type.", ioda_Here());

      // Check the array type's dimensions.
      vector<Dimensions_t> type_dims = typ.getDimensions();
      if (type_dims.size() != 1) throw Exception("Wrong array type rank.", ioda_Here());
      if (type_dims[0] != 2) throw Exception("Wrong array time dimensions.", ioda_Here());

      // Check that the array type's _components_ are unsigned 64-bit ints.
      {
        // Get the base type. I.e. with uint64_t[2], get the decayed type uint64_t.
        Type typ_inner = typ.getBaseType();

        // Check that the base type is an integer.
        if (typ_inner.getClass() != TypeClass::Integer)
          throw Exception("Wrong base type (not an integer).", ioda_Here());

        // Verify that the base type is 64 bits (8 bytes) long.
        if (typ_inner.getSize() != 8)
          throw Exception("Base type is not a 64-bit integer.", ioda_Here());

        // Verify that the base type is unsigned.
        if (typ_inner.isTypeSigned())
          throw Exception("Base type is not an unsigned 64-bit integer.", ioda_Here());
      }

      // For debugging, write the type to the file.
      typ.commitToBackend(f, "Debug_array_type");
    }

    {  // Read and check an attribute
      std::vector<Something_Like_DateTime> check_datetimes;
      f.atts["dates"].read(check_datetimes);
      if (check_datetimes.size() != 3)
        throw Exception("We read the wrong amount of data!", ioda_Here())
          .add("Read #", check_datetimes.size())
          .add<size_t>("Expected #", 3);
      for (size_t i = 0; i < check_datetimes.size(); ++i)
        if (datetimes[i] != check_datetimes[i])
          throw Exception("Attribute equality check failed", ioda_Here());
    }

    {  // Write a variable
      const std::vector<Something_Like_DateTime> datetimes{
        {20210101, 0}, {20210101, 120000}, {20210913, 143000}};
      f.vars.create<Something_Like_DateTime>("datetime", {3})
        .write<Something_Like_DateTime>(datetimes);
    }

    {  // Read and check a variable
      std::vector<Something_Like_DateTime> check_datetimes;
      f.vars["datetime"].read(check_datetimes);
      if (check_datetimes.size() != 3)
        throw Exception("We read the wrong amount of data!", ioda_Here())
          .add("Read #", check_datetimes.size())
          .add<size_t>("Expected #", 3);
      for (size_t i = 0; i < check_datetimes.size(); ++i)
        if (datetimes[i] != check_datetimes[i])
          throw Exception("Attribute equality check failed", ioda_Here());
    }

  } catch (const std::exception& e) {
    ioda::unwind_exception_stack(e);
    return 1;
  }
  return 0;
}
