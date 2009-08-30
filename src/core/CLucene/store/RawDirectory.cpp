/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"

#include "RawDirectory.h"
#include "FSIOFactory.h"
#include "RawIndexInput.h"
#include "RawIndexOutput.h"

CL_NS_DEF(store)

	IOFactory* RawDirectory::defaultIOFactory = new FSIOFactory<RawIndexInput, RawIndexOutput>;

  RawDirectory::RawDirectory(const char* _path, const bool createDir, LockFactory* lockFactory, IOFactory* ioFactory):
   FSDirectory(_path, createDir, lockFactory, ioFactory)
  {
  }

  //static
  FSDirectory* RawDirectory::getDirectory(const char* file, const bool _create, LockFactory* lockFactory, IOFactory* ioFactory){
    return FSDirectory::getDirectory(file, _create, lockFactory, ioFactory);
  }
CL_NS_END
