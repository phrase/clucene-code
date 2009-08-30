/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_store_intl_RAMDirectory_
#define _lucene_store_intl_RAMDirectory_


#include "IndexInput.h"
#include "IndexOutput.h"
#include "RAMDirectory.h"
//#include "Lock.h"
//#include "Directory.h"
//#include "CLucene/util/VoidMap.h"
//#include "CLucene/util/Arrays.h"
#include "CLucene/util/Misc.h"
#include <assert.h>

CL_NS_DEF(store)

	class RAMFile:LUCENE_BASE {
	private:
		struct RAMFileBuffer:LUCENE_BASE {
			uint8_t* _buffer; size_t _len;
			RAMFileBuffer(uint8_t* buf = NULL, size_t len=0) : _buffer(buf), _len(len) {};
			~RAMFileBuffer() { _CLDELETE_LARRAY(_buffer); };
		};


		CL_NS(util)::CLVector<RAMFileBuffer*,CL_NS(util)::Deletor::Object<RAMFileBuffer> > buffers;


		int64_t length;
		RAMDirectory* directory;
		int64_t sizeInBytes;                  // Only maintained if in a directory; updates synchronized on directory

		// This is publicly modifiable via Directory::touchFile(), so direct access not supported
		uint64_t lastModified;


	public:
    DEFINE_MUTEX(THIS_LOCK)

    #ifdef _DEBUG
		const char* filename;
    #endif
		// File used as buffer, in no RAMDirectory
		RAMFile( RAMDirectory* directory=NULL );
		~RAMFile();
		
		// For non-stream access from thread that might be concurrent with writing
		int64_t getLength();
		void setLength( const int64_t _length );
		
		// For non-stream access from thread that might be concurrent with writing
		uint64_t getLastModified();
		void setLastModified( const uint64_t lastModified );
		
		uint8_t* addBuffer( const int32_t size );
		uint8_t* getBuffer( const int32_t index );
		size_t getBufferLen(const int32_t index) const { return buffers[index]->_len; }
		int32_t numBuffers() const;
		uint8_t* newBuffer( const int32_t size );
		
		int64_t getSizeInBytes() const;
	};


	template<typename storage>
	class RAMOutputStream: public storage {		
	protected:
		RAMFile* file;
		bool deleteFile;
		
		uint8_t* currentBuffer;
		int32_t currentBufferIndex;
		
		int32_t bufferPosition;
		int64_t bufferStart;
		int32_t bufferLength;
		
		void switchCurrentBuffer();
		void setFileLength();
				
	public:
		LUCENE_STATIC_CONSTANT(int32_t,BUFFER_SIZE=1024);
		
		RAMOutputStream(RAMFile* f);
		RAMOutputStream();
  	    /** Construct an empty output buffer. */
		virtual ~RAMOutputStream();

		virtual void close();

		int64_t length() const;
    /** Resets this to an empty buffer. */
    void reset();
    /** Copy the current contents of this buffer to the named output. */
    void writeTo(storage* output);
        
  	void writeByte(const uint8_t b);
  	void writeBytes(const uint8_t* b, const int32_t len);

  	void seek(const int64_t pos);
  	
  	void flush();
  	
  	int64_t getFilePointer() const;
    	
		const char* getObjectName();
		static const char* getClassName();   	
   	
	};
	typedef RAMOutputStream<IndexOutput> RAMIndexOutput; //deprecated

	template<typename storage>
	class RAMInputStream:public storage {				
	private:
		RAMFile* file;
		int64_t _length;
		
		uint8_t* currentBuffer;
		int32_t currentBufferIndex;
		
		int32_t bufferPosition;
		int64_t bufferStart;
		int32_t bufferLength;
		
		void switchCurrentBuffer();
		
	protected:
		/** IndexInput methods */
		RAMInputStream(const RAMInputStream& clone);
		
	public:
		LUCENE_STATIC_CONSTANT(int32_t,BUFFER_SIZE=RAMOutputStream<storage>::BUFFER_SIZE);

		RAMInputStream(RAMFile* f);
		~RAMInputStream();
		IndexInput* clone() const;

		void close();
		int64_t length() const;
		
		inline uint8_t readByte();
		void readBytes( uint8_t* dest, const int32_t len );
		
		inline int64_t getFilePointer() const;
		
		void seek(const int64_t pos);
		const char* getDirectoryType() const;
		const char* getObjectName() const;
		static const char* getClassName();
	};
	typedef RAMInputStream<IndexInput> RAMIndexInput; //deprecated

  template<typename storage>
  RAMOutputStream<storage>::~RAMOutputStream(){
	  if ( deleteFile ){
          _CLDELETE(file);
    }else{
     	  file = NULL;
    }
  }

  template<typename storage>
  RAMOutputStream<storage>::RAMOutputStream(RAMFile* f):
	  file(f),
	  deleteFile(false),
	  currentBuffer(NULL),
	  currentBufferIndex(-1),
	  bufferPosition(0),
	  bufferStart(0),
	  bufferLength(0)
  {
  }

  template<typename storage>
  RAMOutputStream<storage>::RAMOutputStream():
    file(_CLNEW RAMFile),
    deleteFile(true),
    currentBuffer(NULL),
    currentBufferIndex(-1),
    bufferPosition(0),
    bufferStart(0),
    bufferLength(0)
  {
  }

  template<typename storage>
  void RAMOutputStream<storage>::writeTo(storage* out){
    flush();
    const int64_t end = file->getLength();
    int64_t pos = 0;
    int32_t p = 0;
    while (pos < end) {
      int32_t length = BUFFER_SIZE;
      int64_t nextPos = pos + length;
      if (nextPos > end) {                        // at the last buffer
        length = (int32_t)(end - pos);
      }
      out->writeBytes(file->getBuffer(p++), length);
      pos = nextPos;
    }
  }

  template<typename storage>
  void RAMOutputStream<storage>::reset(){
	seek((int64_t)0);
    file->setLength((int64_t)0);
  }

  template<typename storage>
  void RAMOutputStream<storage>::close() {
    flush();
  }

  /** Random-at methods */
  template<typename storage>
  void RAMOutputStream<storage>::seek( const int64_t pos ) {
          // set the file length in case we seek back
          // and flush() has not been called yet
	  setFileLength();
	  if ( pos < bufferStart || pos >= bufferStart + bufferLength ) {
		  currentBufferIndex = (int32_t)(pos / BUFFER_SIZE);
		  switchCurrentBuffer();
	  }

	  bufferPosition = (int32_t)( pos % BUFFER_SIZE );
  }

  template<typename storage>
  int64_t RAMOutputStream<storage>::length() const {
    return file->getLength();
  }

  template<typename storage>
  void RAMOutputStream<storage>::writeByte( const uint8_t b ) {
	  if ( bufferPosition == bufferLength ) {
		  currentBufferIndex++;
		  switchCurrentBuffer();
	  }
	  currentBuffer[bufferPosition++] = b;
  }

  template<typename storage>
  void RAMOutputStream<storage>::writeBytes( const uint8_t* b, const int32_t len ) {
	  int32_t srcOffset = 0;

	  while ( srcOffset != len ) {
		  if ( bufferPosition == bufferLength ) {
			  currentBufferIndex++;
			  switchCurrentBuffer();
		  }

		  int32_t remainInSrcBuffer = len - srcOffset;
		  int32_t bytesInBuffer = bufferLength - bufferPosition;
		  int32_t bytesToCopy = bytesInBuffer >= remainInSrcBuffer ? remainInSrcBuffer : bytesInBuffer;

		  memcpy( currentBuffer+bufferPosition, b+srcOffset, bytesToCopy * sizeof(uint8_t) );

		  srcOffset += bytesToCopy;
		  bufferPosition += bytesToCopy;
	  }
  }

  template<typename storage>
  void RAMOutputStream<storage>::switchCurrentBuffer() {

	  if ( currentBufferIndex == file->numBuffers() ) {
		  currentBuffer = file->addBuffer( BUFFER_SIZE );
		  bufferLength = BUFFER_SIZE;
	  } else {
		  currentBuffer = file->getBuffer( currentBufferIndex );
		  bufferLength = file->getBufferLen(currentBufferIndex);
	  }
    assert(bufferLength >=0);//

	  bufferPosition = 0;
	  bufferStart = (int64_t)BUFFER_SIZE * (int64_t)currentBufferIndex;
  }



  template<typename storage>
  void RAMOutputStream<storage>::setFileLength() {
	  int64_t pointer = bufferStart + bufferPosition;
	  if ( pointer > file->getLength() ) {
		  file->setLength( pointer );
	  }
  }

  template<typename storage>
  void RAMOutputStream<storage>::flush() {
	  file->setLastModified( CL_NS(util)::Misc::currentTimeMillis() );
	  setFileLength();
  }

  template<typename storage>
  int64_t RAMOutputStream<storage>::getFilePointer() const {
	  return currentBufferIndex < 0 ? 0 : bufferStart + bufferPosition;
  }


  template<typename storage>
  RAMInputStream<storage>::RAMInputStream(RAMFile* f):
  	file(f),
  	currentBuffer(NULL),
  	currentBufferIndex(-1),
  	bufferPosition(0),
  	bufferStart(0),
  	bufferLength(0)
  {
    _length = f->getLength();

    if ( _length/BUFFER_SIZE >= 0x7FFFFFFFL ) {
    	// TODO: throw exception
    }
  }

  template<typename storage>
  RAMInputStream<storage>::RAMInputStream(const RAMInputStream& other):
    storage(other)
  {
  	file = other.file;
    _length = other._length;
    currentBufferIndex = other.currentBufferIndex;
    currentBuffer = other.currentBuffer;
    bufferPosition = other.bufferPosition;
    bufferStart = other.bufferStart;
    bufferLength = other.bufferLength;
  }

  template<typename storage>
  RAMInputStream<storage>::~RAMInputStream(){
      RAMInputStream::close();
  }

  template<typename storage>
  IndexInput* RAMInputStream<storage>::clone() const
  {
    return _CLNEW RAMInputStream(*this);
  }

  template<typename storage>
  int64_t RAMInputStream<storage>::length() const {
    return _length;
  }

  template<typename storage>
  const char* RAMInputStream<storage>::getDirectoryType() const{
	  return RAMDirectory::getClassName();
  }
	template<typename storage>
	const char* RAMInputStream<storage>::getObjectName() const{ return getClassName(); }
	template<typename storage>
	const char* RAMInputStream<storage>::getClassName(){ return "RAMInputStream"; }

  template<typename storage>
  uint8_t RAMInputStream<storage>::readByte()
  {
	  if ( bufferPosition >= bufferLength ) {
		  currentBufferIndex++;
		  switchCurrentBuffer();
	  }
	  return currentBuffer[bufferPosition++];
  }

  template<typename storage>
  void RAMInputStream<storage>::readBytes( uint8_t* _dest, const int32_t _len ) {

	  uint8_t* dest = _dest;
	  int32_t len = _len;

	  while ( len > 0 ) {
		  if ( bufferPosition >= bufferLength ) {
			  currentBufferIndex++;
			  switchCurrentBuffer();
		  }

		  int32_t remainInBuffer = bufferLength - bufferPosition;
		  int32_t bytesToCopy = len < remainInBuffer ? len : remainInBuffer;
		  memcpy( dest, currentBuffer+bufferPosition, bytesToCopy * sizeof(uint8_t) );

		  dest += bytesToCopy;
		  len -= bytesToCopy;
		  bufferPosition += bytesToCopy;
	  }

  }

  template<typename storage>
  int64_t RAMInputStream<storage>::getFilePointer() const {
	  return currentBufferIndex < 0 ? 0 : bufferStart + bufferPosition;
  }

  template<typename storage>
  void RAMInputStream<storage>::seek( const int64_t pos ) {
	  if ( currentBuffer == NULL || pos < bufferStart || pos >= bufferStart + BUFFER_SIZE ) {
		  currentBufferIndex = (int32_t)( pos / BUFFER_SIZE );
		  switchCurrentBuffer();
	  }
	  bufferPosition = (int32_t)(pos % BUFFER_SIZE);
  }

  template<typename storage>
  void RAMInputStream<storage>::close() {
  }

  template<typename storage>
  void RAMInputStream<storage>::switchCurrentBuffer() {
	  if ( currentBufferIndex >= file->numBuffers() ) {
		  // end of file reached, no more buffers left
		  _CLTHROWA(CL_ERR_IO, "Read past EOF");
	  } else {
		  currentBuffer = file->getBuffer( currentBufferIndex );
		  bufferPosition = 0;
		  bufferStart = (int64_t)BUFFER_SIZE * (int64_t)currentBufferIndex;
		  int64_t bufLen = _length - bufferStart;
		  bufferLength = bufLen > BUFFER_SIZE ? BUFFER_SIZE : static_cast<int32_t>(bufLen);
	  }
    assert (bufferLength >=0);
  }

CL_NS_END
#endif
