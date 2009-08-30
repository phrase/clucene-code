/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_store_BufferedIndexOutput_
#define _lucene_store_BufferedIndexOutput_

#include "CLucene/_ApiHeader.h"
#include "IndexInput.h"
#include "CLucene/util/Misc.h"

CL_NS_DEF(store)

/** Base implementation class for buffered {@link IndexOutput}. */
template<typename storage>
class CLUCENE_EXPORT BufferedIndexOutput : public storage{
public:
	LUCENE_STATIC_CONSTANT(int32_t, BUFFER_SIZE=16384);
private:
	uint8_t* buffer;
	int64_t bufferStart;			  // position in file of buffer
	int32_t bufferPosition;		  // position in buffer

public:
	BufferedIndexOutput();
	virtual ~BufferedIndexOutput();

	/** Writes a single byte.
	* @see IndexInput#readByte()
	*/
	virtual void writeByte(const uint8_t b);

	/** Writes an array of bytes.
	* @param b the bytes to write
	* @param length the number of bytes to write
	* @see IndexInput#readBytes(byte[],int32_t,int32_t)
	*/
	virtual void writeBytes(const uint8_t* b, const int32_t length);

	/** Closes this stream to further operations. */
	virtual void close();

	/** Returns the current position in this file, where the next write will
	* occur.
	* @see #seek(long)
	*/
	int64_t getFilePointer() const;

	/** Sets current position in this file, where the next write will occur.
	* @see #getFilePointer()
	*/
	virtual void seek(const int64_t pos);

	/** The number of bytes in the file. */
	virtual int64_t length() const = 0;

	/** Forces any buffered output to be written. */
	void flush();

protected:
	/** Expert: implements buffer write.  Writes bytes at the current position in
	* the output.
	* @param b the bytes to write
	* @param len the number of bytes to write
	*/
	virtual void flushBuffer(const uint8_t* b, const int32_t len) = 0;
};

  template<typename storage>
  BufferedIndexOutput<storage>::BufferedIndexOutput()
  {
    buffer = _CL_NEWARRAY(uint8_t, BUFFER_SIZE );
    bufferStart = 0;
    bufferPosition = 0;
  }

  template<typename storage>
  BufferedIndexOutput<storage>::~BufferedIndexOutput(){
  	if ( buffer != NULL )
  		close();
  }

  template<typename storage>
  void BufferedIndexOutput<storage>::close(){
    flush();
    _CLDELETE_ARRAY( buffer );

    bufferStart = 0;
    bufferPosition = 0;
  }

  template<typename storage>
  void BufferedIndexOutput<storage>::writeByte(const uint8_t b) {
  	CND_PRECONDITION(buffer!=NULL,"IndexOutput is closed")
    if (bufferPosition >= BUFFER_SIZE)
      flush();
    buffer[bufferPosition++] = b;
  }

  template<typename storage>
  void BufferedIndexOutput<storage>::writeBytes(const uint8_t* b, const int32_t length) {
	  if ( length < 0 )
		  _CLTHROWA(CL_ERR_IllegalArgument, "IO Argument Error. Value must be a positive value.");
	  int32_t bytesLeft = BUFFER_SIZE - bufferPosition;
	  // is there enough space in the buffer?
	  if (bytesLeft >= length) {
		  // we add the data to the end of the buffer
		  memcpy(buffer + bufferPosition, b, length);
		  bufferPosition += length;
		  // if the buffer is full, flush it
		  if (BUFFER_SIZE - bufferPosition == 0)
			  flush();
	  } else {
		  // is data larger then buffer?
		  if (length > BUFFER_SIZE) {
			  // we flush the buffer
			  if (bufferPosition > 0)
				  flush();
			  // and write data at once
			  flushBuffer(b, length);
			  bufferStart += length;
		  } else {
			  // we fill/flush the buffer (until the input is written)
			  int64_t pos = 0; // position in the input data
			  int32_t pieceLength;
			  while (pos < length) {
				  if ( length - pos < bytesLeft )
					pieceLength = (int32_t)(length - pos);
				  else
					pieceLength = bytesLeft;
				  memcpy(buffer + bufferPosition, b + pos, pieceLength);
				  pos += pieceLength;
				  bufferPosition += pieceLength;
				  // if the buffer is full, flush it
				  bytesLeft = BUFFER_SIZE - bufferPosition;
				  if (bytesLeft == 0) {
					  flush();
					  bytesLeft = BUFFER_SIZE;
				  }
			  }
		  }
	  }
  }

  template<typename storage>
  int64_t BufferedIndexOutput<storage>::getFilePointer() const{
    return bufferStart + bufferPosition;
  }

  template<typename storage>
  void BufferedIndexOutput<storage>::seek(const int64_t pos) {
    flush();
    bufferStart = pos;
  }

  template<typename storage>
  void BufferedIndexOutput<storage>::flush() {
    flushBuffer(buffer, bufferPosition);
    bufferStart += bufferPosition;
    bufferPosition = 0;
  }

CL_NS_END
#endif
