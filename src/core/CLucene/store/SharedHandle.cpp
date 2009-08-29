 /*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "_SharedHandle.h"

CL_NS_DEF(store)

  SharedHandle::SharedHandle(const char* path){
  	fhandle = 0;
    _length = 0;
    _fpos = 0;
    strcpy(this->path,path);

#ifndef _CL_DISABLE_MULTITHREADING
	  THIS_LOCK = new _LUCENE_THREADMUTEX;
#endif
  }

  SharedHandle::~SharedHandle() {
    if ( fhandle >= 0 ){
      if ( ::_close(fhandle) != 0 )
        _CLTHROWA(CL_ERR_IO, "File IO Close error");
      else
        fhandle = -1;
    }
  }

CL_NS_END
