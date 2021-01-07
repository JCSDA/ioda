# Adding a Backend Engine (Advanced)

\brief This page describes how the backend engines are implemented.

These are sketched out notes for what has to be part of a fully-working backend. Before starting, you should probably ask the repository maintainers for advice, since they have already written several engines. Implementing a new backend correctly will probably take 1-2 months. Good luck.

Ioda-engines provides three engines currently: HDF5 files, HDF5 in-memory and an in-memory ObsSpace container.

Potential future engines:
- ODB/ODC
- AWS S3 ([based on HDF5 1.12](https://portal.hdfgroup.org/display/HDF5/New+Features+in+HDF5+Release+1.12#NewFeaturesinHDF5Release1.12-vol))

A backend engine needs to provide support for:
- Groups (ioda::Group)
- Attributes (ioda::Attribute) and ioda::Has_Attributes
- The type system (ioda::Type and ioda::detail::Type_Provider)
- Variables (ioda::Variable and ioda::Has_Variables)
- Data selectors (how Variables interact with ioda::Selection)
We recommend that you try to implement these features in the above order, and should aim to pass the various unit tests before progressing to the next feature.

## The general class structure

Ioda-engines using a repeating pattern of inheritance in C++. You will have a ```NAME_Base``` class, one or more ```NAME_backend``` classes, and an end-user-accessible ```NAME``` class.

Take a look at the structure for ioda::Group, ioda::detail::Group_Base, and ioda::detail::Group_Backend. These are all declared in [include/ioda/Group.h](@ref include/ioda/Group.h), and definitions are all in [src/ioda/Group.cpp](@ref src/ioda/Group.cpp).

The base object, ioda::detail::Group_Base is a class that defines the functionality of a Group. Groups provide a few virtual functions (list, exists, create, open, etc.) and class members (atts and vars) that a backend needs to provide implementions for. Group_Base also provides a private opaque shared pointer (backend_) that points to the backend.

The user-accessible object, ioda::Group, directly inherits from ioda::detail::Group_Base, and provides only constructors that pass in a backend, as a shared_ptr. All function calls to ioda::Group get passed back to the backend.

We have this odd split between frontend and backend because we want to separate the backend entirely from the user. The user is exposed to **none** of the backend's details directly. Furthermore, we have to use this pattern if we want to use ioda::Group as a stack-allocated object - this pattern avoids [object slicing](https://en.wikipedia.org/wiki/Object_slicing). It's complicated, but it works.


### What does a backend have to implement?

The base class will have more functions than the backend needs to provide. You only have to override the functions marked ```virtual``` within the base class.

Consider ioda::detail::Attribute_Base. This class has *a lot* of functions. However, a backend really only has to provide four functions:
```
    virtual Attribute write(gsl::span<char> data, const Type& in_memory_dataType) override final;
    virtual Attribute read(gsl::span<char> data, const Type& in_memory_dataType) const override final;
    virtual bool isA(Type lhs) const override final;
    virtual Dimensions getDimensions() const override final;
```

## Adding support for Groups

Groups can be divided into two parts: the base group structure, and the ".atts" and ".vars" members.

### Base group structure

You can think of the base group structure as a tree. Some root node (the root group) acts as
the initial object. It may hold other groups as shared_ptrs. These groups may, in turn, hold their
own children. When a group is destroyed (or if it goes out of scope), its resources are freed.

A good example of this is in Steve's ObsStore backend. See [ioda/private/ioda/ObsStore/Group.hpp](@ref ioda/private/ioda/ObsStore/Group.hpp) and [ioda/src/ioda/Engines/ObsStore/Group.cpp](@ref ioda/src/ioda/Engines/ObsStore/Group.cpp).

### Add a function to create a root group

A root group is the top-level group for a particular instance of a backend.

See [include/ioda/Engines/HH.h](@ref include/ioda/Engines/HH.h) for example functions that
instantiate and return a new backend as a ioda::Group. There are three entry points here, ioda::Engines::HH::createFile, ioda::Engines::HH::openFile, and ioda::Engines::HH::createMemoryFile.

See [include/ioda/Engines/ObsStore.h](@ref include/ioda/Engines/ObsStore.h) for another example. This is much simpler than the HH backend.

### The ".atts" and ".vars" members

In the front end:

These are set in ioda::detail::Group_Base::Group_Base. Basically, a Group is constructed from a provided
backend object. ioda::detail::Group_Base::atts and ioda::detail::Group_Base::vars are set to match the objects that the backend
has provided. See [ioda/src/ioda/Group.cpp](@ref ioda/src/ioda/Group.cpp) for details.

At the back end:

These get linked in the same manner as in the front end. See [ioda/private/ioda/Engines/HH/HH-groups.h](@ref ioda/private/ioda/Engines/HH/HH-groups.h) and
[ioda/private/ioda/Engines/ObsStore/ObsStore-groups.h](@ref ioda/private/ioda/Engines/ObsStore/ObsStore-groups.h) for examples of how these
are linked up. For an early implementation of groups without support for attributes or variables, you
can leave ```.atts``` and ```.vars``` unset, but will eventually link in the objects that you create in later sections.

## Adding support for Attributes and Has_Attributes

See examples in the ObsStore backend.

There are three things that you have to work on:
- Creating a class for your attributes (relatively easy)
- Linking attributes with a group or a variable (also not too bad)
- Proper reading and writing of attributes with the type system (much more difficult)

Due to limitations in C++, the backend storage will probably not be strongly typed. The
frontend "serializes" your data into a byte stream, and this is what you associate with your attribute.

## The type system

The type system is detailed in [ioda/include/ioda/Types/Type.h](@ref ioda/include/ioda/Types/Type.h).
Any backend needs to bind to the type system using the ioda::detail::Type_Provider interface,
which is in [ioda/include/ioda/Types/Type_Provider.h](@ref ioda/include/ioda/Types/Type_Provider.h).

The code that serializes and deserializes the data is located in 
[ioda/include/ioda/Types/Marshalling.h](@ref ioda/include/ioda/Types/Marshalling.h).

You should be able to store all of the fundamental C++ types (listed in Type.h). Array types are
needed to support DateTime objects, and string types are needed to support variable-length strings.

## Adding Variables

The main differences between variables and attributes are that variables are resizable and support
partial reads and writes.

Resizing an object can be costly. You might want to look into implementing chunked storage for
your data.

## Variable data space selectors

Variables support partial reads and writes through the "file_space" and "mem_space" parameters in
their read and write functions. 

TODO: Finish this.
