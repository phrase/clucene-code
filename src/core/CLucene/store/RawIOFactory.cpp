/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/

#include "CLucene/_ApiHeader.h"
#include "RawIOFactory.h"
#include "RawIndexInput.h"
#include "RawIndexOutput.h"
#include "_SharedHandle.h"

CL_NS_DEF(store)

IndexInput* RawIOFactory::newInput(boost::shared_ptr<SharedHandle> const& handle, int32_t __bufferSize) {
    return _CLNEW RawIndexInput(handle, __bufferSize);
}

IndexOutput* RawIOFactory::newOutput(const char* path) {
    return _CLNEW RawIndexOutput(path);
}
CL_NS_END
