/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "FSIndexOutput.h"

#include <fcntl.h>
#ifdef _CL_HAVE_IO_H
        #include <io.h>
#endif
#ifdef _CL_HAVE_SYS_STAT_H
        #include <sys/stat.h>
#endif
#ifdef _CL_HAVE_UNISTD_H
        #include <unistd.h>
#endif
#ifdef _CL_HAVE_DIRECT_H
        #include <direct.h>
#endif
#include <errno.h>

#include "CLucene/util/Misc.h"

CL_NS_USE(util)
CL_NS_DEF(store)

  FSIndexOutput::FSIndexOutput(const char* path){
	//O_BINARY - Opens file in binary (untranslated) mode
	//O_CREAT - Creates and opens new file for writing. Has no effect if file specified by filename exists
	//O_RANDOM - Specifies that caching is optimized for, but not restricted to, random access from disk.
	//O_WRONLY - Opens file for writing only;
	  if ( Misc::dir_Exists(path) )
	    fhandle = _cl_open( path, _O_BINARY | O_RDWR | _O_RANDOM | O_TRUNC, _S_IREAD | _S_IWRITE);
	  else // added by JBP
	    fhandle = _cl_open( path, _O_BINARY | O_RDWR | _O_RANDOM | O_CREAT, _S_IREAD | _S_IWRITE);

	  if ( fhandle < 0 ){
      int err = errno;
      if ( err == ENOENT )
	      _CLTHROWA(CL_ERR_IO, "File does not exist");
      else if ( err == EACCES )
          _CLTHROWA(CL_ERR_IO, "File Access denied");
      else if ( err == EMFILE )
          _CLTHROWA(CL_ERR_IO, "Too many open files");
    }
  }
  FSIndexOutput::~FSIndexOutput(){
	if ( fhandle >= 0 ){
	  try {
        FSIndexOutput::close();
	  }catch(CLuceneError& err){
	    //ignore IO errors...
	    if ( err.number() != CL_ERR_IO )
	        throw;
	  }
	}
  }

  /** output methods: */
  void FSIndexOutput::flushBuffer(const uint8_t* b, const int32_t size) {
	  CND_PRECONDITION(fhandle>=0,"file is not open");
      if ( size > 0 && _write(fhandle,b,size) != size )
        _CLTHROWA(CL_ERR_IO, "File IO Write error");
  }
  void FSIndexOutput::close() {
    try{
      BufferedIndexOutput<IndexOutput>::close();
    }catch(CLuceneError& err){
	    //ignore IO errors...
	    if ( err.number() != CL_ERR_IO )
	        throw;
    }

    if ( ::_close(fhandle) != 0 )
      _CLTHROWA(CL_ERR_IO, "File IO Close error");
    else
      fhandle = -1; //-1 now indicates closed
  }

  void FSIndexOutput::seek(const int64_t pos) {
    CND_PRECONDITION(fhandle>=0,"file is not open");
    BufferedIndexOutput<IndexOutput>::seek(pos);
	int64_t ret = fileSeek(fhandle,pos,SEEK_SET);
	if ( ret != pos ){
      _CLTHROWA(CL_ERR_IO, "File IO Seek error");
	}
  }
  int64_t FSIndexOutput::length() const {
	  CND_PRECONDITION(fhandle>=0,"file is not open");
	  return fileSize(fhandle);
  }

CL_NS_END
