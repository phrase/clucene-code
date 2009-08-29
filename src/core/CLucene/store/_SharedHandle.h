/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_store_SharedHandle_
#define _lucene_store_SharedHandle_

#include "CLucene/_ApiHeader.h"

CL_NS_DEF(store)

   /** class for sharing file handles for {@link lucene::store::FSIndexInput}.
    * This reduces number of file handles we need, and it means
    * we dont have to use file tell (which is slow) before doing
    * a read.
    * TODO: get rid of this and dup/fctnl or something like that...
    * @see FSIndexInput
    */
		class SharedHandle {
		public:
			int32_t fhandle;
			int64_t _length;
			int64_t _fpos;
			DEFINE_MUTEX(*THIS_LOCK)
			char path[CL_MAX_DIR]; //todo: this is only used for cloning, better to get information from the fhandle
			SharedHandle(const char* path);
			~SharedHandle();
		};
CL_NS_END
#endif
