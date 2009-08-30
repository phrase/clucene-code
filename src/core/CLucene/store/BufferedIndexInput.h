/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_store_BufferedIndexInput_
#define _lucene_store_BufferedIndexInput_

#include "CLucene/_ApiHeader.h"
#include "IndexInput.h"
#include "BufferedIndexOutput.h"
#include "CLucene/util/Misc.h"

CL_NS_DEF(store)

   /** Abstract base class for input from a file in a {@link Directory}.  A
   * random-access input stream.  Used for all Lucene index input operations.
   * @see Directory
   * @see IndexOutput
   */
	template<typename storage>
	class CLUCENE_EXPORT BufferedIndexInput: public storage{
	private:
		uint8_t* buffer; //array of bytes
		void refill();
	protected:
		int32_t bufferSize;				//size of the buffer
		int64_t bufferStart;			  // position in file of buffer
		int32_t bufferLength;			  // end of valid l_byte_ts
		int32_t bufferPosition;		  // next uint8_t to read

      /** Returns a clone of this stream.
      *
      * <p>Clones of a stream access the same data, and are positioned at the same
      * point as the stream they were cloned from.
      *
      * <p>Expert: Subclasses must ensure that clones may be positioned at
      * different points in the input from each other and from the stream they
      * were cloned from.
      */
		BufferedIndexInput(const BufferedIndexInput& clone);
		BufferedIndexInput(int32_t bufferSize = -1);
	public:
		LUCENE_STATIC_CONSTANT(int32_t, BUFFER_SIZE=LUCENE_STREAM_BUFFER_SIZE);

		virtual ~BufferedIndexInput();
		virtual IndexInput* clone() const = 0;
		void close();
		inline uint8_t readByte(){
			if (bufferPosition >= bufferLength)
				refill();

			return buffer[bufferPosition++];
		}
		void readBytes(uint8_t* b, const int32_t len);
		void readBytes(uint8_t* b, const int32_t len, bool useBuffer);
		int64_t getFilePointer() const;
		void seek(const int64_t pos);

		void setBufferSize( int32_t newSize );

		const char* getObjectName();
		static const char* getClassName();

	protected:
      /** Expert: implements buffer refill.  Reads bytes from the current position
      * in the input.
      * @param b the array to read bytes into
      * @param offset the offset in the array to start storing bytes
      * @param length the number of bytes to read
      */
		virtual void readInternal(uint8_t* b, const int32_t len) = 0;

      /** Expert: implements seek.  Sets current position in this file, where the
      * next {@link #readInternal(byte[],int32_t,int32_t)} will occur.
      * @see #readInternal(byte[],int32_t,int32_t)
      */
		virtual void seekInternal(const int64_t pos) = 0;
	};

template<typename storage>
BufferedIndexInput<storage>::BufferedIndexInput(int32_t _bufferSize):
		buffer(NULL),
		bufferSize(_bufferSize>=0?_bufferSize:CL_NS(store)::BufferedIndexOutput<storage>::BUFFER_SIZE),
		bufferStart(0),
		bufferLength(0),
		bufferPosition(0)
  {
  }

  template<typename storage>
  BufferedIndexInput<storage>::BufferedIndexInput(const BufferedIndexInput& other):
  	storage(other),
    buffer(NULL),
    bufferSize(other.bufferSize),
    bufferStart(other.bufferStart),
    bufferLength(other.bufferLength),
    bufferPosition(other.bufferPosition)
  {
    /* DSR: Does the fact that sometime clone.buffer is not NULL even when
    ** clone.bufferLength is zero indicate memory corruption/leakage?
    **   if ( clone.buffer != NULL) { */
    if (other.bufferSize != 0 && other.buffer != NULL) {
      buffer = _CL_NEWARRAY(uint8_t,bufferSize);
      memcpy(buffer,other.buffer,bufferSize * sizeof(uint8_t));
    }
  }

	template<typename storage>
	const char* BufferedIndexInput<storage>::getObjectName(){ return getClassName(); }
	template<typename storage>
	const char* BufferedIndexInput<storage>::getClassName(){ return "BufferedIndexInput"; }
		
  template<typename storage>
  void BufferedIndexInput<storage>::readBytes(uint8_t* b, const int32_t len){
    readBytes(b, len, true);
  }
  template<typename storage>
  void BufferedIndexInput<storage>::readBytes(uint8_t* _b, const int32_t _len, bool useBuffer){
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
        if(after > this->length())
          _CLTHROWA(CL_ERR_IO, "read past EOF");
        readInternal(b, len);
        bufferStart = after;
        bufferPosition = 0;
        bufferLength = 0;                    // trigger refill() on read
      }
    }
  }

  template<typename storage>
  int64_t BufferedIndexInput<storage>::getFilePointer() const{
    return bufferStart + bufferPosition;
  }

  template<typename storage>
  void BufferedIndexInput<storage>::seek(const int64_t pos) {
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
  template<typename storage>
  void BufferedIndexInput<storage>::close(){
    _CLDELETE_ARRAY(buffer);
    bufferLength = 0;
    bufferPosition = 0;
    bufferStart = 0;
  }


  template<typename storage>
  BufferedIndexInput<storage>::~BufferedIndexInput(){
    BufferedIndexInput::close();
  }

  template<typename storage>
  void BufferedIndexInput<storage>::refill() {
    int64_t start = bufferStart + bufferPosition;
    int64_t end = start + bufferSize;
    if (end > this->length())				  // don't read past EOF
      end = this->length();
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

  template<typename storage>
  void BufferedIndexInput<storage>::setBufferSize( int32_t newSize ) {

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
#endif
