/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "RawIndexOutput.h"

CL_NS_DEF(store)

  void RawIndexOutput::writeChars(const TCHAR* s, const int32_t length){
    writeBytes(reinterpret_cast<const uint8_t*>(s), length * sizeof(TCHAR) / sizeof(uint8_t));
  }

CL_NS_END
