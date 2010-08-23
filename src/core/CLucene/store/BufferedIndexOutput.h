/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_store_BufferedIndexOutput_
#define _lucene_store_BufferedIndexOutput_

#include "IndexOutput.h"

CL_NS_DEF(store)

/** Base implementation class for buffered {@link IndexOutput}. */
class CLUCENE_EXPORT BufferedIndexOutput : public IndexOutput{
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

CL_NS_END
#endif
