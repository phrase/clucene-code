/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_store_RawIndexOutput_
#define _lucene_store_RawIndexOutput_

#include "FSIndexOutput.h"

CL_NS_DEF(store)

class CLUCENE_EXPORT RawIndexOutput: public FSIndexOutput{
public:
	RawIndexOutput(const char* path);

	/** Writes a sequence of raw TCHAR encoded characters from a string.
	* @param s the source of the characters
	* @param start the first character in the sequence
	* @param length the number of characters in the sequence
	* @see RawIndexInput#readChars(char[],int32_t,int32_t)
	*/
	void writeChars(const TCHAR* s, const int32_t length);

};

CL_NS_END
#endif
