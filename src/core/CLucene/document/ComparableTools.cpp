/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"

#include "ComparableTools.h"
#include "CLucene/util/Misc.h"

CL_NS_DEF(document)

std::basic_string<TCHAR> ComparableTools<int64_t>::valueToString(int64_t l)
{
	if (l == LUCENE_INT64_MIN_SHOULDBE) {
		// special case, because long is not symetric around zero
		return MIN_STRING_VALUE();
	}

	TCHAR buf[STR_SIZE + 1];
	if (l < 0) {
		buf[0] = NEGATIVE_PREFIX[0];
		l = LUCENE_INT64_MAX_SHOULDBE + l + 1;
	} else {
		buf[0] = POSITIVE_PREFIX[0];
	}

	TCHAR tmp[STR_SIZE];
	_i64tot(l, tmp, NUMBERTOOLS_RADIX);
	size_t len = _tcslen(tmp);
	_tcscpy(buf+(STR_SIZE-len),tmp);
	for ( size_t i=1;i<STR_SIZE-len;i++ )
		buf[i] = (int)'0';

	buf[STR_SIZE] = 0;

	return buf;
}

int64_t ComparableTools<int64_t>::stringToValue(std::basic_string<TCHAR> const& str) {
	TCHAR const* chars = str.c_str();
	if (chars == NULL) {
		_CLTHROWA(CL_ERR_NullPointer,"string cannot be null");
	}
	if (_tcslen(chars) != STR_SIZE) {
		_CLTHROWA(CL_ERR_NumberFormat,"string is the wrong size");
	}

	if (_tcscmp(chars, MIN_STRING_VALUE().c_str()) == 0) {
		return LUCENE_INT64_MIN_SHOULDBE;
	}

	TCHAR prefix = chars[0];

	TCHAR* sentinel = NULL;
	int64_t l = _tcstoi64(++chars, &sentinel, NUMBERTOOLS_RADIX);

	if (prefix == POSITIVE_PREFIX[0]) {
		// nop
	} else if (prefix == NEGATIVE_PREFIX[0]) {
		l = l - LUCENE_INT64_MAX_SHOULDBE - 1;
	} else {
		_CLTHROWA(CL_ERR_NumberFormat,"string does not begin with the correct prefix");
	}

	return l;
}

ComparableTools<int64_t>::~ComparableTools(){
}

CL_NS_END
