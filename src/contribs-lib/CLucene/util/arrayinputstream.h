/*------------------------------------------------------------------------------
* Copyright (C) 2009 Isidor Zeuner
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef CLUCENE_UTIL_ARRAYINPUTSTREAM_H
#define CLUCENE_UTIL_ARRAYINPUTSTREAM_H

#include "CLucene/_ApiHeader.h"
#include "CLucene/util/Array.h"

CL_NS_DEF(util)

template<typename base>
class CLUCENE_CONTRIBS_EXPORT ArrayInputStream : public base {
public:
	ArrayInputStream(ArrayBase<typename base::element_type> const* data);
	int32_t read(const typename base::element_type*& start, int32_t min, int32_t max);
	int64_t skip(int64_t ntoskip);
	int64_t position();
	size_t size();
private:
	ArrayBase<typename base::element_type> const* data;
	int64_t current_position;
};

template<typename base>
ArrayInputStream<base>::ArrayInputStream(ArrayBase<typename base::element_type> const* data) :
data(data),
current_position(0) {
}

template<typename base>
int32_t ArrayInputStream<base>::read(const typename base::element_type*& start, int32_t min, int32_t max) {
	int32_t to_read = min;
	int32_t readable = data->length - current_position;
	if (readable < to_read) {
		to_read = readable;
	}
	start = data->values + current_position;
	current_position += to_read;
	return to_read;
}
	
template<typename base>
int64_t ArrayInputStream<base>::skip(int64_t ntoskip) {
	int64_t to_skip = ntoskip;
	int64_t skippable = data->length - current_position;
	if (skippable < to_skip) {
		to_skip = skippable;
	}
	current_position += to_skip;
	return to_skip;
}

template<typename base>
int64_t ArrayInputStream<base>::position() {
	return current_position;
}
	
template<typename base>
size_t ArrayInputStream<base>::size() {
	return data->length;
}
CL_NS_END
#endif
