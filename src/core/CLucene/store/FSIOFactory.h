/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_store_FSIOFactory_
#define _lucene_store_FSIOFactory_

#include "CLucene/_ApiHeader.h"
#include "CLucene/store/IOFactory.h"
#include "FSIndexInput.h"
#include "FSIndexOutput.h"
#include <boost/shared_ptr.hpp>

CL_NS_DEF(store)

template<typename input, typename output>
class CLUCENE_EXPORT FSIOFactory : public IOFactory {
public:
	bool openInput(const char* path, IndexInput*& ret, CLuceneError& error, int32_t bufferSize=-1);
	IndexOutput* newOutput(const char* path);
	static const char* getClassName();
	const char* getObjectName() const;

};

template<typename input, typename output>
bool FSIOFactory<input, output>::openInput(const char* path, IndexInput*& ret, CLuceneError& error, int32_t bufferSize) {
    return FSIndexInput<input>::open(path, ret, error, bufferSize);
}

template<typename input, typename output>
IndexOutput* FSIOFactory<input, output>::newOutput(const char* path) {
    return _CLNEW FSIndexOutput<output>(path);
}

template<typename input, typename output>
const char* FSIOFactory<input, output>::getClassName(){
    return "FSIOFactory";
}

template<typename input, typename output>
const char* FSIOFactory<input, output>::getObjectName() const{
    return getClassName();
}                     

CL_NS_END
#endif
