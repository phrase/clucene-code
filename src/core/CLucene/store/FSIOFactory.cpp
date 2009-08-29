/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/

#include "CLucene/_ApiHeader.h"
#include "FSIOFactory.h"
#include "FSIndexInput.h"
#include "FSIndexOutput.h"
#include <boost/shared_ptr.hpp>

CL_NS_DEF(store)


bool FSIOFactory::openInput(const char* path, IndexInput*& ret, CLuceneError& error, int32_t bufferSize) {
    return FSIndexInput::open(this, path, ret, error, bufferSize);
}

IndexInput* FSIOFactory::newInput(boost::shared_ptr<SharedHandle> const& handle, int32_t __bufferSize) {
    return _CLNEW FSIndexInput(handle, __bufferSize);
}

IndexOutput* FSIOFactory::newOutput(const char* path) {
    return _CLNEW FSIndexOutput(path);
}
CL_NS_END
