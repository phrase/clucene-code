/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"

#include "NumberTools.h"
#include "CLucene/util/Misc.h"

CL_NS_DEF(document)

std::basic_string<TCHAR> NumberTools::longToString(int64_t l)
{
	return ComparableTools<int64_t>::valueToString(l);
}

int64_t NumberTools::stringToLong(std::basic_string<TCHAR> const& str) {
	return ComparableTools<int64_t>::stringToValue(str);
}

NumberTools::~NumberTools(){
}

CL_NS_END
