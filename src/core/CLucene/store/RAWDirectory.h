/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_store_RAWDirectory_
#define _lucene_store_RAWDirectory_

#include "CLucene/util/VoidMap.h"
#include "FSDirectory.h"
#include "RawIOFactory.h"

CL_NS_DEF(store)

	/**
	* Experimental:
  * A raw string {@link Directory} implementation. Not compatible
  * with JLucene. Should be faster, because no utf-8-esque encoding and
  * decoding needs to be done.
  * NOTE: There is no detection of format, so if you write with this,
  * then opening with a normal FSDirectory may have unpredictable results
  * (and vice-versa). Same goes for opening with instances using different
  * TCHAR definitions
	*
	*/
	class CLUCENE_EXPORT RAWDirectory:public FSDirectory{
	private:
		static RawIOFactory defaultIOFactory;
	protected:
		RAWDirectory(const char* path, const bool createDir, LockFactory* lockFactory=NULL, IOFactory* ioFactory = &defaultIOFactory);
	public:
    /**
    Returns the directory instance for the named location.

    Do not delete this instance, only use close, otherwise other instances
    will lose this instance.

    <p>Directories are cached, so that, for a given canonical path, the same
    FSDirectory instance will always be returned.  This permits
    synchronization on directories.

    @param file the path to the directory.
    @param create if true, create, or erase any existing contents.
    @return the FSDirectory for the named file.
    */
		static FSDirectory* getDirectory(const char* file, const bool create=false, LockFactory* lockFactory=NULL, IOFactory* ioFactory = &defaultIOFactory);

	};
CL_NS_END
#endif
