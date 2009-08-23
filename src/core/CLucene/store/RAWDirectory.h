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

CL_NS_DEF(store)

	/**
	* Experimental:
  * A raw string {@link Directory} implementation. Not compatible
  * with JLucene. Should be faster, because no utf-8-esque encoding and
  * decoding needs to be done.
  * NOTE: There is no detection of format, so if you write with this,
  * then opening with a normal FSDirectory may have unpredictable results
  * (and vice-versa)
	*
	*/
	class CLUCENE_EXPORT RAWDirectory:public FSDirectory{
	protected:
		RAWDirectory(const char* path, const bool createDir, LockFactory* lockFactory=NULL);
	public:
	  ///Destructor - only call this if you are sure the directory
	  ///is not being used anymore. Otherwise use the ref-counting
	  ///facilities of _CLDECDELETE
		virtual ~RAWDirectory();

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
		static RAWDirectory* getDirectory(const char* file, const bool create=false, LockFactory* lockFactory=NULL);

		/// Returns a stream reading an existing file.
		virtual bool openInput(const char* name, IndexInput*& ret, CLuceneError& err, int32_t bufferSize=-1);

		/// Creates a new, empty file in the directory with the given name.
		///	Returns a stream writing this file.
		virtual IndexOutput* createOutput(const char* name);
	};
CL_NS_END
#endif
