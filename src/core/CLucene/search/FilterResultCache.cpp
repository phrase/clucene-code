/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "FilterResultCache.h"
#include "CachingWrapperFilter.h"
#include "CLucene/index/IndexReader.h"

CL_NS_DEF(search)
CL_NS_USE(index)
CL_NS_USE(util)


template<class T>
FilterResultCache<T>::FilterResultCache( bool bDeleteResults )
{
    this->bDeleteResults = bDeleteResults;
}

template<class T>
FilterResultCache<T>::~FilterResultCache()
{
}

template<class T>
T* FilterResultCache<T>::getResult( CL_NS(index)::IndexReader* reader )
{
	SCOPED_LOCK_MUTEX( cache_LOCK )
	ResultHolder<T> * cached = cache.get( reader );
	if( cached != NULL )
        return cached->result;
	
    T * result = fiter->( reader );
	ResultHolder * holder = _CLNEW ResultHolder( result, bDeleteResults );
	cache.put( reader, holder );
	return result;
}

template<class T>
void FilterResultCache<T>::closeCallback( CL_NS(index)::IndexReader* reader, void* )
{
	SCOPED_LOCK_MUTEX( cache_LOCK )
	cache.remove( reader );
}

CL_NS_END
