 /*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "IndexInput.h"
#include "IndexOutput.h"
#include "CLucene/util/Misc.h"

CL_NS_DEF(store)
CL_NS_USE(util)

	IndexInput::IndexInput():
		NamedObject()
	{
	}
	IndexInput::~IndexInput()
	{
	}
	IndexInput::IndexInput(const IndexInput& /*other*/)
	{
	}

  int32_t IndexInput::readInt() {
    int32_t b = (readByte() << 24);
    b |= (readByte() << 16);
    b |= (readByte() <<  8);
    return (b | readByte());
  }

  int32_t IndexInput::readVInt() {
    uint8_t b = readByte();
    int32_t i = b & 0x7F;
    for (int32_t shift = 7; (b & 0x80) != 0; shift += 7) {
      b = readByte();
      i |= (b & 0x7F) << shift;
    }
    return i;
  }

  int64_t IndexInput::readLong() {
    int64_t i = ((int64_t)readInt() << 32);
    return (i | ((int64_t)readInt() & 0xFFFFFFFFL));
  }

  int64_t IndexInput::readVLong() {
    uint8_t b = readByte();
    int64_t i = b & 0x7F;
    for (int32_t shift = 7; (b & 0x80) != 0; shift += 7) {
      b = readByte();
      i |= (((int64_t)b) & 0x7FL) << shift;
    }
    return i;
  }

  void IndexInput::skipChars( const int32_t count) {
	for (int32_t i = 0; i < count; i++) {
		TCHAR b = readByte();
		if ((b & 0x80) == 0) {
			// Do Nothing.
		} else if ((b & 0xE0) != 0xE0) {
			readByte();
		} else {
			readByte();
			readByte();
		}
	}
}

	#ifdef _UCS2
  int32_t IndexInput::readString(char* buffer, const int32_t maxLength){
  	TCHAR* buf = _CL_NEWARRAY(TCHAR,maxLength);
    int32_t ret = -1;
  	try{
	  	ret = readString(buf,maxLength);
	  	STRCPY_TtoA(buffer,buf,ret+1);
  	}_CLFINALLY ( _CLDELETE_CARRAY(buf); )
  	return ret;
  }
  #endif

  int32_t IndexInput::readString(TCHAR* buffer, const int32_t maxLength){
    int32_t len = readVInt();
		int32_t ml=maxLength-1;
    if ( len >= ml ){
      readChars(buffer, 0, ml);
      buffer[ml] = 0;
      //we have to finish reading all the data for this string!
      if ( len-ml > 0 ){
				//seek(getFilePointer()+(len-ml)); <- that was the wrong way to "finish reading"
				skipChars(len-ml);
		  }
      return ml;
    }else{
      readChars(buffer, 0, len);
      buffer[len] = 0;
      return len;
    }
  }

   TCHAR* IndexInput::readString(){
    int32_t len = readVInt();

    if ( len == 0){
      return stringDuplicate(LUCENE_BLANK_STRING);
    }

    TCHAR* ret = _CL_NEWARRAY(TCHAR,len+1);
    readChars(ret, 0, len);
    ret[len] = 0;

    return ret;
  }

  void IndexInput::readBytes( uint8_t* b, const int32_t len, bool /*useBuffer*/) {
    // Default to ignoring useBuffer entirely
    readBytes(b, len);
  }

  void IndexInput::readChars( TCHAR* buffer, const int32_t start, const int32_t len) {
    const int32_t end = start + len;
    TCHAR b;
    for (int32_t i = start; i < end; ++i) {
      b = readByte();
      if ((b & 0x80) == 0) {
        b = (b & 0x7F);
      } else if ((b & 0xE0) != 0xE0) {
        b = (((b & 0x1F) << 6)
          | (readByte() & 0x3F));
      } else {
		  b = ((b & 0x0F) << 12) | ((readByte() & 0x3F) << 6);
		  b |= (readByte() & 0x3F);
      }
      buffer[i] = b;
	}
  }

CL_NS_END

