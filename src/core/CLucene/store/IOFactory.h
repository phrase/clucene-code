/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_store_IOFactory_
#define _lucene_store_IOFactory_

#include "IndexInput.h"
#include "IndexOutput.h"
#include "_SharedHandle.h"
#include "CLucene/util/Equators.h"
#include <boost/shared_ptr.hpp>

CL_NS_DEF(store)

   /** Abstract base factory class supplying input and output streams to
   * a {@link lucene::store::Directory}.
   * @see IndexInput
   * @see IndexOutput
   */   
class CLUCENE_EXPORT IOFactory : public CL_NS(util)::NamedObject {
public:
	virtual bool openInput(const char* path, IndexInput*& ret, CLuceneError& error, int32_t bufferSize=-1) = 0;
	virtual IndexOutput* newOutput(const char* path) = 0;
};
CL_NS_END
#endif
