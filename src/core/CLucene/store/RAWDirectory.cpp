/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"

#include "RAWDirectory.h"

CL_NS_DEF(store)

	RawIOFactory RAWDirectory::defaultIOFactory;

  RAWDirectory::RAWDirectory(const char* _path, const bool createDir, LockFactory* lockFactory, IOFactory* ioFactory):
   FSDirectory(_path, createDir, lockFactory, ioFactory)
  {
  }

  //static
  FSDirectory* RAWDirectory::getDirectory(const char* file, const bool _create, LockFactory* lockFactory, IOFactory* ioFactory){
    return FSDirectory::getDirectory(file, _create, lockFactory, ioFactory);
  }
CL_NS_END
