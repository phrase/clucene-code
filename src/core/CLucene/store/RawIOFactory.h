/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_store_RawIOFactory_
#define _lucene_store_RawIOFactory_

#include "CLucene/store/FSIOFactory.h"

CL_NS_DEF(store)

class CLUCENE_EXPORT RawIOFactory : public FSIOFactory {
public:
	IndexInput* newInput(boost::shared_ptr<SharedHandle> const& handle, int32_t __bufferSize);
	IndexOutput* newOutput(const char* path);
};
CL_NS_END
#endif
