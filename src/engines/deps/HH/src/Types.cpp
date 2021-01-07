/*
 * (C) Copyright 2017-2020 Ryan Honeyager (ryan@honeyager.info)
 *
 * This software is licensed under the terms of the BSD 2-Clause License.
 * For details, see the LICENSE file.
 */
#include "HH/Types.hpp"


namespace HH {
	namespace _impl {
		size_t COMPAT_strncpy_s(
			char* dest, size_t destSz,
			const char* src, size_t srcSz)
		{
			/** \brief Safe char array copy. Uses strncpy_s if available.
			\returns the number of characters actually written.
			\param dest is the pointer to the destination. Always null terminated.
			\param destSz is the size of the destination buller, including the trailing null character.
			\param src is the pointer to the source. Characters from src are copied either until the
			first null character or until srcSz. Note that null termination comes later.
			\param srcSz is the max size of the source buffer.
			**/
			if (!dest || !src) throw;  // HH_throw.add("Reason", "Null pointer passed to function.");
#ifdef HH_USING_SECURE_STRINGS
			strncpy_s(dest, destSz, src, srcSz);
			return strnlen_s(dest, destSz);
#else
			if (destSz == 0) throw;  // HH_throw.add("Reason", "Invalid destination size.");
			// See https://devblogs.microsoft.com/oldnewthing/20170927-00/?p=97095
			// Unfortunately, we can't really detect pointer range overlaps in a system-independent
			// manner. This is why we want to use the secure string functions if at all possible.
			if (srcSz <= destSz) {
				strncpy(dest, src, srcSz);
				if (dest[srcSz] != '\0') throw;  // HH_throw.add("Reason", "Non-terminated null copy.");
			} else {
				strncpy(dest, src, destSz);
				// Additionally, throw on null-string truncation.

				// Not using strchr because of its memory-unsafe nature.
				for (size_t i = 0; i < destSz - 1; ++i)
				{
					if (i < srcSz) {
						if (dest[i] == '\0' && src[i] == '\0') break; // First null hit. Success.
						if (dest[i] == '\0' && src[i] != '\0') throw;  // HH_throw.add("Reason", "Truncated array copy error.");
					}
					else throw;  // HH_throw.add("Reason", "Null not reached by end of source!");
				}
			}
			dest[destSz - 1] = 0;
			for (size_t i = 0; i < destSz; ++i) {
				if (dest[i] == '\0') return i;
			}
			throw;  // HH_throw.add("Reason", "Truncated array copy error.");
#endif
		}

	} // namespace _impl
	namespace Types {
		using namespace HH::Handles;

		

	} // namespace Types
} // namespace HH

