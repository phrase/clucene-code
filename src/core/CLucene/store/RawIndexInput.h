/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_store_RawIndexInput_
#define _lucene_store_RawIndexInput_

#include "FSIndexInput.h"

CL_CLASS_DEF(store,RawIOFactory)

CL_NS_DEF(store)

	class CLUCENE_EXPORT RawIndexInput: public FSIndexInput {
		friend class RawIOFactory;
	protected:
		RawIndexInput(boost::shared_ptr<SharedHandle> const& handle, int32_t __bufferSize);
	public:
		IndexInput* clone() const;

		/** Reads raw TCHAR encoded characters into an array.
		* @param buffer the array to read characters into
		* @param start the offset in the array to start storing characters
		* @param length the number of characters to read
		* @see RawIndexOutput#writeChars(String,int32_t,int32_t)
		*/
		void readChars( TCHAR* buffer, const int32_t start, const int32_t len);

		void skipChars( const int32_t count);
	};
CL_NS_END
#endif
