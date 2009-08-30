/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_store_FSIndexInput_
#define _lucene_store_FSIndexInput_

#include "BufferedIndexInput.h"
#include "FSDirectory.h"
#include "_SharedHandle.h"
#include "IOFactory.h"
#include <boost/shared_ptr.hpp>

CL_CLASS_DEF(store,FSIOFactory)

CL_NS_DEF(store)

   /** class for input from a file in a {@link lucene::store::FSDirectory}.  A
   * random-access input stream.
   * @see FSDirectory
   * @see IndexInput
   */
	class CLUCENE_EXPORT FSIndexInput:public BufferedIndexInput {
		friend class FSIOFactory;
		boost::shared_ptr<SharedHandle> handle;
		int64_t _pos;
	protected:
		FSIndexInput(boost::shared_ptr<SharedHandle> const& handle, int32_t __bufferSize):
			BufferedIndexInput(__bufferSize)
		{
			this->_pos = 0;
			this->handle = handle;
		};
		FSIndexInput(const FSIndexInput& clone);
	public:
		static bool open(IOFactory* ioFactory, const char* path, IndexInput*& ret, CLuceneError& error, int32_t bufferSize=-1);
		~FSIndexInput();

		IndexInput* clone() const;
		void close();
		int64_t length() const { return handle.get()->_length; }

		const char* getDirectoryType() const{ return FSDirectory::getClassName(); }
    const char* getObjectName() const{ return getClassName(); }
    static const char* getClassName() { return "FSIndexInput"; }
	protected:
		// Random-access methods
		void seekInternal(const int64_t position);
		// IndexInput methods
		void readInternal(uint8_t* b, const int32_t len);
	};
CL_NS_END
#endif
