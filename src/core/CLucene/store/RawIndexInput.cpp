 /*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "RawIndexInput.h"

CL_NS_DEF(store)

  RawIndexInput::RawIndexInput(boost::shared_ptr<SharedHandle> const& handle, int32_t __bufferSize) :
    FSIndexInput(handle, __bufferSize) {
}

  IndexInput* RawIndexInput::clone() const
  {
    return _CLNEW RawIndexInput(*this);
  }

  void RawIndexInput::skipChars( const int32_t count) {
	seek(getFilePointer()+(count * sizeof(TCHAR) / sizeof(uint8_t)));
}

  void RawIndexInput::readChars( TCHAR* buffer, const int32_t start, const int32_t len) {
    readBytes(reinterpret_cast<uint8_t*>(buffer + start), len * sizeof(TCHAR) / sizeof(uint8_t));
  }

CL_NS_END

