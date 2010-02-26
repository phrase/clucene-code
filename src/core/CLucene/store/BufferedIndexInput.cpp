/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "BufferedIndexInput.h"
#include "IndexOutput.h"
#include "IndexInput.h"
#include "CLucene/util/Misc.h"

CL_NS_USE(util)
CL_NS_DEF(store)



BufferedIndexInput::BufferedIndexInput(int32_t _bufferSize):
		buffer(NULL),
		bufferSize(_bufferSize>=0?_bufferSize:CL_NS(store)::BufferedIndexOutput::BUFFER_SIZE),
		bufferStart(0),
		bufferLength(0),
		bufferPosition(0)
  {
  }

  BufferedIndexInput::BufferedIndexInput(const BufferedIndexInput& other):
  	IndexInput(other),
    buffer(NULL),
    bufferSize(other.bufferSize),
    bufferStart(other.bufferStart),
    bufferLength(other.bufferLength),
    bufferPosition(other.bufferPosition)
  {
    /* DSR: Does the fact that sometime clone.buffer is not NULL even when
    ** clone.bufferLength is zero indicate memory corruption/leakage?
    **   if ( clone.buffer != NULL) { */
    if (other.bufferLength != 0 && other.buffer != NULL) {
      buffer = _CL_NEWARRAY(uint8_t,bufferLength);
      memcpy(buffer,other.buffer,bufferLength * sizeof(uint8_t));
    }
  }

  void BufferedIndexInput::readBytes(uint8_t* b, const int32_t len){
    readBytes(b, len, true);
  }
  void BufferedIndexInput::readBytes(uint8_t* _b, const int32_t _len, bool useBuffer){
    int32_t len = _len;
    uint8_t* b = _b;

    if(len <= (bufferLength-bufferPosition)){
      // the buffer contains enough data to satisfy this request
      if(len>0) // to allow b to be null if len is 0...
        memcpy(b, buffer + bufferPosition, len);
      bufferPosition+=len;
    } else {
      // the buffer does not have enough data. First serve all we've got.
      int32_t available = bufferLength - bufferPosition;
      if(available > 0){
        memcpy(b, buffer + bufferPosition, available);
        b += available;
        len -= available;
        bufferPosition += available;
      }
      // and now, read the remaining 'len' bytes:
      if (useBuffer && len<bufferSize){
        // If the amount left to read is small enough, and
        // we are allowed to use our buffer, do it in the usual
        // buffered way: fill the buffer and copy from it:
        refill();
        if(bufferLength<len){
          // Throw an exception when refill() could not read len bytes:
          memcpy(b, buffer, bufferLength);
          _CLTHROWA(CL_ERR_IO, "read past EOF");
        } else {
          memcpy(b, buffer, len);
          bufferPosition=len;
        }
      } else {
        // The amount left to read is larger than the buffer
        // or we've been asked to not use our buffer -
        // there's no performance reason not to read it all
        // at once. Note that unlike the previous code of
        // this function, there is no need to do a seek
        // here, because there's no need to reread what we
        // had in the buffer.
        int64_t after = bufferStart+bufferPosition+len;
        if(after > length())
          _CLTHROWA(CL_ERR_IO, "read past EOF");
        readInternal(b, len);
        bufferStart = after;
        bufferPosition = 0;
        bufferLength = 0;                    // trigger refill() on read
      }
    }
  }

  int64_t BufferedIndexInput::getFilePointer() const{
    return bufferStart + bufferPosition;
  }

  void BufferedIndexInput::seek(const int64_t pos) {
    if ( pos < 0 )
      _CLTHROWA(CL_ERR_IO, "IO Argument Error. Value must be a positive value.");
    if (pos >= bufferStart && pos < (bufferStart + bufferLength))
      bufferPosition = (int32_t)(pos - bufferStart);  // seek within buffer
    else {
      bufferStart = pos;
      bufferPosition = 0;
      bufferLength = 0;				  // trigger refill() on read()
      seekInternal(pos);
    }
  }
  void BufferedIndexInput::close(){
    _CLDELETE_ARRAY(buffer);
    bufferLength = 0;
    bufferPosition = 0;
    bufferStart = 0;
  }


  BufferedIndexInput::~BufferedIndexInput(){
    BufferedIndexInput::close();
  }

  void BufferedIndexInput::refill() {
    int64_t start = bufferStart + bufferPosition;
    int64_t end = start + bufferSize;
    if (end > length())				  // don't read past EOF
      end = length();
    bufferLength = (int32_t)(end - start);
    if (bufferLength <= 0)
      _CLTHROWA(CL_ERR_IO, "IndexInput read past EOF");

    if (buffer == NULL){
      buffer = _CL_NEWARRAY(uint8_t,bufferSize);		  // allocate buffer lazily
    }
    readInternal(buffer, bufferLength);


    bufferStart = start;
    bufferPosition = 0;
  }

  void BufferedIndexInput::setBufferSize( int32_t newSize ) {

	  if ( newSize != bufferSize ) {
		  bufferSize = newSize;
		  if ( buffer != NULL ) {

			  uint8_t* newBuffer = _CL_NEWARRAY( uint8_t, newSize );
			  int32_t leftInBuffer = bufferLength - bufferPosition;
			  int32_t numToCopy;

			  if ( leftInBuffer > newSize ) {
				  numToCopy = newSize;
			  } else {
				  numToCopy = leftInBuffer;
			  }

			  memcpy( (void*)newBuffer, (void*)(buffer + bufferPosition), numToCopy );

			  bufferStart += bufferPosition;
			  bufferPosition = 0;
			  bufferLength = numToCopy;

			  _CLDELETE_ARRAY( buffer );
			  buffer = newBuffer;

		  }
	  }

  }

CL_NS_END

