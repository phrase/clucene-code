/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_store_FSIndexOutput_
#define _lucene_store_FSIndexOutput_

#include "BufferedIndexOutput.h"

CL_NS_DEF(store)

/** class for output to a file in a Directory.  A random-access output
* stream.
* @see FSDirectory
* @see IndexOutput
*/
	class CLUCENE_EXPORT FSIndexOutput: public BufferedIndexOutput {
	private:
		int32_t fhandle;
	protected:
		// output methods:
		void flushBuffer(const uint8_t* b, const int32_t size);
	public:
		FSIndexOutput(const char* path);
		~FSIndexOutput();

		// output methods:
		void close();

		// Random-access methods
		void seek(const int64_t pos);
		int64_t length() const;
	};
CL_NS_END
#endif
