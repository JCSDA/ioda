#pragma once
/*
 * (C) Copyright 2017-2020 Ryan Honeyager (ryan@honeyager.info)
 *
 * This software is licensed under the terms of the BSD 2-Clause License.
 * For details, see the LICENSE file.
 */
#include "../Types.hpp"
#include <array>
#include <complex>
#include <vector>

namespace HH {
	namespace Types {
		template <class ComplexDataType>
		HH_hid_t GetHDF5TypeComplex()
		{
			typedef typename ComplexDataType::value_type value_type;
			// Complex number is a compound datatype of two objects.
			return GetHDF5Type<value_type, 1>({ 2 });
		}

#define HH_SPECIALIZE_COMPLEX(x) \
template<> inline HH_hid_t GetHDF5Type< x >(std::initializer_list<hsize_t>, void*) { return GetHDF5TypeComplex< x >(); }
		HH_SPECIALIZE_COMPLEX(std::complex<double>);
		HH_SPECIALIZE_COMPLEX(std::complex<float>);
#undef HH_SPECIALIZE_COMPLEX

		// Complex number accessor:
		// Strictly, the real and imaginary parts are internal to the complex number. They must be
		// marshalled.

		// TODO(ryan): FINISH!!!!!!!!!!!
		template <class ComplexDataType>
		struct Object_Accessor_Complex
		{
		private:
			typedef typename ComplexDataType::value_type value_type;
			std::vector<std::array<value_type, 2> > _buf;
		public:
			explicit Object_Accessor_Complex(ssize_t sz = -1) {}
			/// \brief Converts an object into a void* array that HDF5 can natively understand.
			/// \note The shared_ptr takes care of "deallocation" when we no longer need the "buffer".
			const void* serialize(::gsl::span<const ComplexDataType> d)
			{
				return (const void*)d.data();
				//return std::shared_ptr<const void>((const void*)d.data(), [](const void*) {});
			}
			/// \brief Gets the size of the buffer needed to store the object from HDF5. Used
			/// in variable-length string / complex object reads.
			/// \note For POD objects, we do not have to allocate a buffer.
			/// \returns Size needed. If negative, then we can directly write to the object, 
			/// sans allocation or deallocation.
			ssize_t getFromBufferSize() {
				return -1;
			}
			/// \brief Allocates a buffer that HDF5 can read/write into; used later as input data for object construction.
			/// \note For POD objects, we can directly write to the object.
			void marshalBuffer(ComplexDataType * objStart) { } //_buffer = static_cast<void*>(objStart); }
			/// \brief Construct an object from an HDF5-provided data stream, 
			/// and deallocate any temporary buffer.
			/// \note For trivial (POD) objects, there is no need to do anything.
			void deserialize(ComplexDataType *objStart) { }
			void freeBuffer() {}
		};
	} // namespace Types
} // namespace HH

