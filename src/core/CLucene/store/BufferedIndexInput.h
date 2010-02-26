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
	class CLUCENE_EXPORT BufferedIndexInput: public IndexInput{
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

		const char* getObjectName(){ return getClassName(); }
		static const char* getClassName(){ return "BufferedIndexInput"; }

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
  
CL_NS_END
#endif
