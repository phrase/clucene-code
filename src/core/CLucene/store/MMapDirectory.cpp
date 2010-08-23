/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "FSDirectory.h"
#include "_MMapDirectory.h"
#include "CLucene/util/Misc.h"
#include "CLucene/util/Array.h"

#ifdef _CL_HAVE_SYS_MMAN_H
	#include <sys/mman.h>
#endif
#ifdef _CL_HAVE_WINERROR_H
	#include <winerror.h>
#endif

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

#if defined(_CL_HAVE_FUNCTION_MAPVIEWOFFILE)
	typedef int HANDLE;
	
	#define GENERIC_READ                     (0x80000000L)
  #define FILE_SHARE_READ                 0x00000001  
  #define OPEN_EXISTING       3
  #define PAGE_READONLY          0x02     
  #define SECTION_MAP_READ    0x0004
  #define FILE_MAP_READ       SECTION_MAP_READ

  typedef struct  _SECURITY_ATTRIBUTES
  {
      _cl_dword_t nLength;
      void* lpSecurityDescriptor;
      bool bInheritHandle;
  }	SECURITY_ATTRIBUTES;

  extern "C" __declspec(dllimport) _cl_dword_t __stdcall GetFileSize( HANDLE hFile, _cl_dword_t* lpFileSizeHigh );
    
  extern "C" __declspec(dllimport) bool __stdcall UnmapViewOfFile( void* lpBaseAddress );

  extern "C" __declspec(dllimport) bool __stdcall CloseHandle( HANDLE hObject );
  extern "C" __declspec(dllimport) HANDLE __stdcall CreateFileA(
	const char* lpFileName,
	_cl_dword_t dwDesiredAccess,
	_cl_dword_t dwShareMode,
	SECURITY_ATTRIBUTES* lpSecurityAttributes,
	_cl_dword_t dwCreationDisposition,
	_cl_dword_t dwFlagsAndAttributes,
	HANDLE hTemplateFile
  );
  extern "C" __declspec(dllimport) HANDLE __stdcall CreateFileMappingA(
      HANDLE hFile,
      SECURITY_ATTRIBUTES* lpFileMappingAttributes,
      _cl_dword_t flProtect,
      _cl_dword_t dwMaximumSizeHigh,
      _cl_dword_t dwMaximumSizeLow,
      const char* lpName
  );
  extern "C" __declspec(dllimport) void* __stdcall MapViewOfFile(
      HANDLE hFileMappingObject,
      _cl_dword_t dwDesiredAccess,
      _cl_dword_t dwFileOffsetHigh,
      _cl_dword_t dwFileOffsetLow,
      _cl_dword_t dwNumberOfBytesToMap
  );
  extern "C" __declspec(dllimport) _cl_dword_t __stdcall GetLastError();
#endif

CL_NS_USE(util)
CL_NS_DEF(store)

class MMapMapping{
public:
#if defined(_CL_HAVE_FUNCTION_MAPVIEWOFFILE)
	HANDLE mmaphandle;
	HANDLE fhandle;
#elif defined(_CL_HAVE_FUNCTION_MMAP)
	int fhandle;
#else
  #error no mmap implementation set
#endif
	uint8_t* data;
	int32_t length;
};

void MMapMapping_Close(MMapMapping& mmap){
#if defined(_CL_HAVE_FUNCTION_MAPVIEWOFFILE)
	if ( mmap.data != NULL ){
		if ( ! UnmapViewOfFile(mmap.data) ){
			CND_PRECONDITION( false, "UnmapViewOfFile(data) failed"); //todo: change to rich error
		}
	}

	if ( mmap.mmaphandle != NULL ){
		if ( ! CloseHandle(mmap.mmaphandle) ){
			CND_PRECONDITION( false, "CloseHandle(mmaphandle) failed");
		}
	}
	if ( mmap.fhandle != NULL ){
		if ( !CloseHandle(mmap.fhandle) ){
			CND_PRECONDITION( false, "CloseHandle(fhandle) failed");
		}
	}
	mmap.mmaphandle = NULL;
	mmap.fhandle = NULL;
#else
	if ( mmap.data != NULL )
  		::munmap(mmap.data, mmap.length);
  	if ( mmap.fhandle > 0 )
  		::close(mmap.fhandle);
  	mmap.fhandle = 0;
#endif
	  mmap.data = NULL;
}
void MMapMapping_Clone(MMapMapping& to, const MMapMapping& clone){
	#if defined(_CL_HAVE_FUNCTION_MAPVIEWOFFILE)
		  to.mmaphandle = NULL;
		  to.fhandle = NULL;
	#endif
	  to.data = clone.data;

	  //clone the file length
	  to.length  = clone.length;
}

void MMapMapping_Create(MMapMapping& mmap, const char* path, int64_t offset = 0, int32_t length = -1){
  //todo: honour start and length...
  mmap.data = NULL;
  mmap.length = 0;
#if defined(_CL_HAVE_FUNCTION_MAPVIEWOFFILE)
  mmap.mmaphandle = NULL;
  mmap.fhandle = CreateFileA(path,GENERIC_READ,FILE_SHARE_READ, 0,OPEN_EXISTING,0,0);
  
  //Check if a valid fhandle was retrieved
  if (mmap.fhandle < 0){
	  _cl_dword_t err = GetLastError();
    if ( err == ERROR_FILE_NOT_FOUND )
      _CLTHROWA(CL_ERR_IO, "File does not exist");
    else if ( err == ERROR_ACCESS_DENIED )
      _CLTHROWA(ERROR_ACCESS_DENIED, "File Access denied");
    else if ( err == ERROR_TOO_MANY_OPEN_FILES )
      _CLTHROWA(CL_ERR_IO, "Too many open files");
	else
		_CLTHROWA(CL_ERR_IO, "File IO Error");
  }

  _cl_dword_t dummy=0;
  
  if ( length < 0 ){
    mmap.length = GetFileSize(mmap.fhandle, &dummy);
  }else{
    mmap.length = length;
  }

  if ( mmap.length > 0 ){
		mmap.mmaphandle = CreateFileMappingA(mmap.fhandle,NULL,PAGE_READONLY,0,0,NULL);
		if ( mmap.mmaphandle != NULL ){
      _cl_dword_t nHigh = (_cl_dword_t) ((offset & _ILONGLONG(0xFFFFFFFF00000000)) >> 32);
      _cl_dword_t nLow = (_cl_dword_t) (offset & _ILONGLONG(0x00000000FFFFFFFF));

			void* address = MapViewOfFile(mmap.mmaphandle,FILE_MAP_READ,nHigh,nLow,mmap.length);
			if ( address != NULL ){
				mmap.data = (uint8_t*)address;
				return; //SUCCESS!
			}
		}
		
		//failure:
		int errnum = GetLastError(); 
		CloseHandle(mmap.mmaphandle);

		char* lpMsgBuf=strerror(errnum);
		size_t len = strlen(lpMsgBuf)+80;
		char* errstr = _CL_NEWARRAY(char, len); 
		cl_sprintf(errstr, len, "MMapIndexInput::MMapIndexInput failed with error %d: %s", len, errnum, lpMsgBuf); 

		_CLTHROWA_DEL(CL_ERR_IO,errstr);
  }

#else //_CL_HAVE_FUNCTION_MAPVIEWOFFILE
 	mmap.fhandle = ::open (path, O_RDONLY);
	if (mmap.fhandle < 0){
		_CLTHROWA(CL_ERR_IO,strerror(errno));	
	}else{
		// stat it
		struct stat sb;
		if (::fstat (mmap.fhandle, &sb)){
			_CLTHROWA(CL_ERR_IO,strerror(errno));
		}else{
			// get length from stat
      if ( length < 0 )
			  mmap.length = sb.st_size;
      else
        mmap.length = length;
			
			// mmap the file
			void* address = ::mmap(0, mmap.length, PROT_READ, MAP_SHARED, mmap.fhandle, offset);
			if (address == MAP_FAILED){
				_CLTHROWA(CL_ERR_IO,strerror(errno));
			}else{
				mmap.data = (uint8_t*)address;
			}
		}
	}
#endif
}

class MMapDirectory::MMapIndexInput::Internal: LUCENE_BASE{
public:
	int64_t pos;
	bool isClone;
	MMapMapping mmap;

	Internal():
  		pos(0),
  		isClone(false)
  	{
  	}
    ~Internal(){
    }
  };

  MMapDirectory::MMapIndexInput::MMapIndexInput(const char* path):
	    _internal(_CLNEW Internal)
	{
	//Func - Constructor.
	//       Opens the file named path
	//Pre  - path != NULL
	//Post - if the file could not be opened  an exception is thrown.

	  CND_PRECONDITION(path != NULL, "path is NULL");
		MMapMapping_Create(_internal->mmap, path);
  }

  MMapDirectory::MMapIndexInput::MMapIndexInput(const MMapIndexInput& clone): IndexInput(clone){
  //Func - Constructor
  //       Uses clone for its initialization
  //Pre  - clone is a valide instance of FSIndexInput
  //Post - The instance has been created and initialized by clone
     _internal = _CLNEW Internal;
     
     MMapMapping_Clone(_internal->mmap, clone._internal->mmap);

	  _internal->pos = clone._internal->pos;

	  //Keep in mind that this instance is a clone
	  _internal->isClone = true;
  }

  uint8_t MMapDirectory::MMapIndexInput::readByte(){
	  return *(_internal->mmap.data+(_internal->pos++));
  }

  void MMapDirectory::MMapIndexInput::readBytes(uint8_t* b, const int32_t len){
	  memcpy(b, _internal->mmap.data+_internal->pos, len);
	  _internal->pos+=len;
  }
  int32_t MMapDirectory::MMapIndexInput::readVInt(){
	  uint8_t b = *(_internal->mmap.data+(_internal->pos++));
	  int32_t i = b & 0x7F;
	  for (int shift = 7; (b & 0x80) != 0; shift += 7) {
	    b = *(_internal->mmap.data+(_internal->pos++));
	    i |= (b & 0x7F) << shift;
	  }
	  return i;
  }
  int64_t MMapDirectory::MMapIndexInput::getFilePointer() const{
	  return _internal->pos;
  }
  void MMapDirectory::MMapIndexInput::seek(const int64_t pos){
	  this->_internal->pos=pos;
  }
  int64_t MMapDirectory::MMapIndexInput::length() const{ 
  	return _internal->mmap.length; 
  }

  MMapDirectory::MMapIndexInput::~MMapIndexInput(){
  //Func - Destructor
  //Pre  - True
  //Post - The file for which this instance is responsible has been closed.
  //       The instance has been destroyed

	  close();
	_CLDELETE(_internal);
  }

  IndexInput* MMapDirectory::MMapIndexInput::clone() const
  {
    return _CLNEW MMapIndexInput(*this);
  }
  void MMapDirectory::MMapIndexInput::close()  {
		if ( !_internal->isClone ){
			MMapMapping_Close(_internal->mmap);
	  }
	  _internal->pos = 0;
  }

/*
  class MMapDirectory::MultiMMapIndexInput::Internal{
  public:
    ValueArray<MMapMapping*> buffers;
  	const int64_t length;
  
    int32_t curBufIndex;
    const int32_t maxBufSize;
  
    uint8_t* curBuf; // redundant for speed: buffers[curBufIndex]
    int32_t curBufPosition;
    int32_t curAvail; // redundant for speed: (bufSizes[curBufIndex] - curBuf.position())

    Internal(int64_t _length, const int32_t _maxBufSize):
      length(_length),
      maxBufSize(_maxBufSize)
    {
    }
    ~Internal(){
      for ( size_t i=0;i<buffers.length;i++ ){
        MMapMapping_Close(*buffers.values[i]);
        _CLDELETE(buffers.values[i]);
      }
    }
  };

  MMapDirectory::MultiMMapIndexInput::MultiMMapIndexInput(const MultiMMapIndexInput& copy):
    IndexInput(copy),
    _internal(_CLNEW Internal(copy._internal->length, copy._internal->maxBufSize))
  {
    _internal->buffers.resize(copy._internal->buffers.length);
xx
    // No need to clone bufSizes.
    // Since most clones will use only one buffer, duplicate() could also be
    // done lazy in clones, eg. when adapting curBuf.
    for (int32_t bufNr = 0; bufNr < _internal->buffers.length; bufNr++) {
      _internal->buffers[bufNr] = copy._internal->buffers[bufNr]; //duplicate
    }
    try {
      this->seek(copy.getFilePointer());
    } catch(IOException ioe) {
      RuntimeException newException = new RuntimeException(ioe);
      newException.initCause(ioe);
      throw newException;
    };
  }

  MMapDirectory::MultiMMapIndexInput::MultiMMapIndexInput(const char* path, int32_t maxBufSize):
    _internal(_CLNEW Internal(Misc::file_Size(path), maxBufSize))
  {
    if (maxBufSize <= 0)
      _CLTHROWA(CL_ERR_IllegalArgument, (string("Non positive maxBufSize: ") + Misc::toString(maxBufSize)).c_str() );
    
    if ((_internal->length / _internal->maxBufSize) > LUCENE_INT32_MAX_SHOULDBE)
      _CLTHROWA(CL_ERR_IllegalArgument, (string("RandomAccessFile too big for maximum buffer size: ")
         + string(path)).c_str());
    
    int32_t nrBuffers = (_internal->length / _internal->maxBufSize);
    if ((nrBuffers * _internal->maxBufSize) < _internal->length) nrBuffers++;
    
    _internal->buffers.resize(nrBuffers);
    
    int64_t bufferStart = 0;
    for (int32_t bufNr = 0; bufNr < nrBuffers; bufNr++) { 
      int32_t bufSize = (_internal->length > (bufferStart + _internal->maxBufSize))
        ? _internal->maxBufSize
        : (int32_t) (_internal->length - bufferStart);

      MMapMapping* mmap = new MMapMapping;
      MMapMapping_Create(*mmap, path, bufferStart, bufSize);
      this->_internal->buffers[bufNr] = mmap;
      bufferStart += bufSize;
    }
    seek(0L);
  }
    
  MMapDirectory::MultiMMapIndexInput::~MultiMMapIndexInput(){
    _CLDELETE(_internal);
  }

  uint8_t MMapDirectory::MultiMMapIndexInput::readByte(){
    // Performance might be improved by reading ahead into an array of
    // eg. 128 bytes and readByte() from there.
    if (_internal->curAvail == 0) {
      _internal->curBufIndex++;
      MMapMapping* mmap = _internal->buffers[_internal->curBufIndex]; // index out of bounds when too many bytes requested
      _internal->curBuf = mmap->data; 
      _internal->curBufPosition = 0;
      _internal->curAvail = mmap->length;
    }
    _internal->curAvail--;
    return _internal->curBuf[_internal->curBufPosition++];
  }

  void MMapDirectory::MultiMMapIndexInput::readBytes(uint8_t* b, const int32_t _len) {
    int32_t len = _len;
    uint8_t* offset = b;
    
    while (len > _internal->curAvail) {
      memcpy(offset, _internal->curBuf, _internal->curAvail);
      _internal->curBufPosition -= _internal->curAvail;
      len -= _internal->curAvail;
      offset += _internal->curAvail;
      _internal->curBufIndex++;
      MMapMapping* mmap = _internal->buffers[_internal->curBufIndex]; // index out of bounds when too many bytes requested
      _internal->curBuf = mmap->data;
      _internal->curBufPosition = 0;
      _internal->curAvail = mmap->length;
    }
    memcpy(offset, _internal->curBuf, len);
    _internal->curBufPosition += len;
    _internal->curAvail -= len;
  }

  int64_t MMapDirectory::MultiMMapIndexInput::getFilePointer() const{
    return (_internal->curBufIndex * (int64_t) _internal->maxBufSize) + _internal->curBufPosition;
  }

  void MMapDirectory::MultiMMapIndexInput::seek(int64_t pos){
    _internal->curBufIndex = (int32_t) (pos / _internal->maxBufSize);
    MMapMapping* mmap = _internal->buffers[_internal->curBufIndex];
    _internal->curBuf = mmap->data;
    int32_t bufOffset = (int32_t) (pos - (_internal->curBufIndex * _internal->maxBufSize));
    _internal->curBufPosition = bufOffset;
    _internal->curAvail = mmap->length - bufOffset;
  }

  int64_t MMapDirectory::MultiMMapIndexInput::length() const {
    return _internal->length;
  }

  IndexInput* MMapDirectory::MultiMMapIndexInput::clone() const{
    return _CLNEW MultiMMapIndexInput(*this);
  }

  void MMapDirectory::MultiMMapIndexInput::close(){
  }
*/





  MMapDirectory::MMapDirectory()
  {
  }

  MMapDirectory::~MMapDirectory(){
  }

  //todo: would this be bigger on 64bit systems?. i suppose it would be...test first
  #define MAX_MMAP_BUF LUCENE_INT32_MAX_SHOULDBE

	/// Returns a stream reading an existing file.
  bool MMapDirectory::openInput(const char* name, IndexInput*& ret, CLuceneError& error, int32_t /*bufferSize*/){
    char fl[CL_MAX_DIR];
    priv_getFN(fl, name);
    int64_t len = Misc::file_Size(fl);
    if ( len < 0 ){
      error.set(CL_ERR_IO, "File does not exist");
      return false;
    }
    try{
      if ( len < MAX_MMAP_BUF ){
		    ret = _CLNEW MMapIndexInput( fl );
        return true;
      }else{
		  //  ret = _CLNEW MultiMMapIndexInput( fl, MAX_MMAP_BUF );
        error.set(0,"Not implemented");
        return false;
      }
    }catch(CLuceneError& err){
      error.set(err.number(), err.what());
      return false;
    }
    
  }
  const char* MMapDirectory::getClassName(){
    return "MMapDirectory";
  }
  const char* MMapDirectory::getObjectName() const{
    return getClassName();
  }
CL_NS_END
