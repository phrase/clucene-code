/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_store_FSDirectory_
#define _lucene_store_FSDirectory_


#include "GenericFSDirectory.h"
#include "IndexInput.h"
#include "IndexOutput.h"

CL_NS_DEF(store)

class CLUCENE_EXPORT FSDirectory : public GenericFSDirectory<
    BufferedIndexInput,
    BufferedIndexOutput
> {
};

CL_NS_END
#endif
