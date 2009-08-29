/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_store_FSIOFactory_
#define _lucene_store_FSIOFactory_

#include "CLucene/store/IOFactory.h"

CL_NS_DEF(store)

class CLUCENE_EXPORT FSIOFactory : public IOFactory {
public:
	bool openInput(const char* path, IndexInput*& ret, CLuceneError& error, int32_t bufferSize=-1);
	IndexOutput* newOutput(const char* path);
};
CL_NS_END
#endif
