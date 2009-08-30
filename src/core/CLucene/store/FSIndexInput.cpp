 /*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "FSIndexInput.h"
#include "BufferedIndexOutput.h"

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

CL_NS_DEF(store)
CL_NS_USE(util)

	bool FSIndexInput::open(IOFactory* ioFactory, const char* path, IndexInput*& ret, CLuceneError& error, int32_t __bufferSize )    {
	//Func - Constructor.
	//       Opens the file named path
	//Pre  - path != NULL
	//Post - if the file could not be opened  an exception is thrown.

	  CND_PRECONDITION(path != NULL, "path is NULL");

	  if ( __bufferSize == -1 )
		  __bufferSize = CL_NS(store)::BufferedIndexOutput<IndexOutput>::BUFFER_SIZE;
	  boost::shared_ptr<SharedHandle> handle(_CLNEW SharedHandle(path));

	  //Open the file
	  handle.get()->fhandle  = ::_cl_open(path, _O_BINARY | O_RDONLY | _O_RANDOM, _S_IREAD );

	  //Check if a valid handle was retrieved
	  if (handle.get()->fhandle >= 0){
		  //Store the file length
		  handle.get()->_length = fileSize(handle.get()->fhandle);
		  if ( handle.get()->_length == -1 )
	  		error.set( CL_ERR_IO,"fileStat error" );
		  else{
			  handle.get()->_fpos = 0;
			  ret = ioFactory->newInput(handle, __bufferSize);
			  return true;
		  }
	  }else{
		  int err = errno;
      if ( err == ENOENT )
	      error.set(CL_ERR_IO, "File does not exist");
      else if ( err == EACCES )
        error.set(CL_ERR_IO, "File Access denied");
      else if ( err == EMFILE )
        error.set(CL_ERR_IO, "Too many open files");
      else
      	error.set(CL_ERR_IO, "Could not open file");
	  }
#ifndef _CL_DISABLE_MULTITHREADING
    delete handle.get()->THIS_LOCK;
#endif
	  return false;
  }

  FSIndexInput::FSIndexInput(const FSIndexInput& other): BufferedIndexInput<IndexInput>(other){
  //Func - Constructor
  //       Uses clone for its initialization
  //Pre  - clone is a valide instance of FSIndexInput
  //Post - The instance has been created and initialized by clone
	  if ( other.handle.get() == NULL )
		  _CLTHROWA(CL_ERR_NullPointer, "other handle is null");

	  SCOPED_LOCK_MUTEX(*other.handle.get()->THIS_LOCK)
	  handle = other.handle;
	  _pos = other.handle.get()->_fpos; //note where we are currently...
  }

  FSIndexInput::~FSIndexInput(){
  //Func - Destructor
  //Pre  - True
  //Post - The file for which this instance is responsible has been closed.
  //       The instance has been destroyed

	  FSIndexInput::close();
  }

  IndexInput* FSIndexInput::clone() const
  {
    return _CLNEW FSIndexInput(*this);
  }
  void FSIndexInput::close()  {
	BufferedIndexInput<IndexInput>::close();
#ifndef _CL_DISABLE_MULTITHREADING
	if ( handle.get() != NULL ){
		boost::shared_ptr<SharedHandle> for_locking = handle;
		SCOPED_LOCK_MUTEX(*for_locking.get()->THIS_LOCK)
		handle.reset();
	}
#else
	handle.reset();
#endif
  }

  void FSIndexInput::seekInternal(const int64_t position)  {
	CND_PRECONDITION(position>=0 &&position<handle.get()->_length,"Seeking out of range")
	_pos = position;
  }

/** IndexInput methods */
void FSIndexInput::readInternal(uint8_t* b, const int32_t len) {
	CND_PRECONDITION(handle.get()!=NULL,"shared file handle has closed");
	CND_PRECONDITION(handle.get()->fhandle>=0,"file is not open");
	SCOPED_LOCK_MUTEX(*handle.get()->THIS_LOCK)

	if ( handle.get()->_fpos != _pos ){
		if ( fileSeek(handle.get()->fhandle,_pos,SEEK_SET) != _pos ){
			_CLTHROWA( CL_ERR_IO, "File IO Seek error");
		}
		handle.get()->_fpos = _pos;
	}

	bufferLength = _read(handle.get()->fhandle,b,len); // 2004.10.31:SF 1037836
	if (bufferLength == 0){
		_CLTHROWA(CL_ERR_IO, "read past EOF");
	}
	if (bufferLength == -1){
		//if (EINTR == errno) we could do something else... but we have
		//to guarantee some return, or throw EOF

		_CLTHROWA(CL_ERR_IO, "read error");
	}
	_pos+=bufferLength;
	handle.get()->_fpos=_pos;
}
CL_NS_END
