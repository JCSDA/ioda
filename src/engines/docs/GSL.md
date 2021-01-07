The Guideline Support Library (GSL) {#GSL}
======================================

\brief This page describes the GSL and how it is used.

Throughout the source and headers, you might 
have noticed that the code is peppered with objects
in the gsl namespace - entities like gsl::not_null and 
gsl::span. The [Guideline Support Library](https://github.com/Microsoft/GSL)
is a library that contains functions and types that are
suggested for use by the [C++ Core Guidelines](https://github.com/isocpp/CppCoreGuidelines)
that are maintained by the [Standard C++ Foundation](https://isocpp.org/).
We aim to follow these best practices, and adherence to these
guidelines should help ensure that the code is long-lasting and portable.

The gsl headers are located in the deps/gsl-single/include/gsl subdirectory. They
are released under the MIT license, and are redistributed here because 
they are not widely distributed to different platforms.

## Summary of the Different GSL Templates and Macros

### gsl::not_null<T>

C and C++ pointers may be used:
- To pass a single object by address
- To indicate arrays
- To provide "output" parameters to functions
- To indicate "optional" parameters

This is horribly vague, and it leads to programming mistakes.
gsl::not_null<T>(obj) ensures that obj is a non-null pointer.
It checks at runtime, and throws an error if a null pointer is encountered.
By placing this check in the declaration of a function, the check is 
consistently performed. This is also an aid to debugging, whereby you
can easily ensure that a null pointer does not enter into a group of nested,
high performance functions.

Of course, it does not check against passing an uninitialized pointer. Smart
pointers (or a smart compiler with all warnings enabled) can help mitigate this.

### gsl::span<T>

This allows you to safely pass a range of objects to a function.

- A common, unsafe, way to pass an array of objects to a function is to just use a
NULL-terminated pointer. But, what if you do not remember to ensure NULL-termination?
- You could also pass a pointer to the start of an object, along with a size parameter.
However, you the size variable is decoupled from the actual array, which allows for 
mistakes.

This is why C++ has iterators. However, iterators are usually employed only in standard
object containers. We could require that the passed objects be withing a standard container,
like a std::vector, but then we are forcing library users into our programming style. 
Sometimes, a vector is not the appropriate container, and we want to avoid excessive
re-shuffling and copying of objects.

A gsl::span<T> object has iterators, size(), begin() and end() functions. They are
trivially constructable.

### gsl::owner<T>

Dynamically-allocated memory should be freed when it is no longer in use. Smart pointers,
like std::unique_ptr<T> and std::shared_ptr<T> provide facilities for freeing an object
when its last reference is gone. However, smart pointers are not always appropriate.

gsl::owner<T> provides a way for a programmer to keep track of how ownership of a pointer
should transfer between function calls.

### Expects(...)

This is a better version of assert. It is always useful to check assumptions, and using
Expects helps to clarify your intent to others who may read your code.

The Expects macro can either throw an error or terminate.
This is set using the GSL_THROW_ON_CONTRACT_VIOLATION macro. Any throws inherit from std::logic_error,
which inherits from std::exception.

### Ensures(...)

This is similar to Expects, but expresses an assert call as a post condition. That is, at the
end of a function, the condition must be respected or an error will occur.
