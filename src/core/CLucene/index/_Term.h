/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_index_internal_Term_
#define _lucene_index_internal_Term_

#include "Term.h"
#include <functional>
#include <boost/shared_ptr.hpp>

CL_NS_DEF(index)


class Term_Equals:public CL_NS_STD(binary_function)<const Term*,const Term*,bool>
{
public:
	bool operator()( const Term* val1, const Term* val2 ) const{
		return val1->equals(val2);
	}
};

class Term_Compare:LUCENE_BASE, public CL_NS(util)::Compare::_base //<Term*>
{
public:
	bool operator()( Term* t1, Term* t2 ) const{
		return ( t1->compareTo(t2) < 0 );
	}
	size_t operator()( Term* t ) const{
		return t->hashCode();
	}
};

class Term_Equals_Shared:public CL_NS_STD(binary_function)<const boost::shared_ptr<const Term>,const boost::shared_ptr<const Term>,bool>
{
public:
	bool operator()( const boost::shared_ptr<const Term>& val1, const boost::shared_ptr<const Term>& val2 ) const{
		return val1.get()->equals(val2.get());
	}
};

class Term_Compare_Shared:LUCENE_BASE, public CL_NS(util)::Compare::_base //<Term*>
{
public:
	bool operator()( const boost::shared_ptr<Term>& t1, const boost::shared_ptr<Term>& t2 ) const{
		return ( t1.get()->compareTo(t2.get()) < 0 );
	}
	size_t operator()( const boost::shared_ptr<Term>& t ) const{
		return t.get()->hashCode();
	}
};

class Term_UnorderedCompare:LUCENE_BASE, public CL_NS(util)::Compare::_base //<Term*>
{
public:
	bool operator()( Term* t1, Term* t2 ) const{
		return ( t1->hashedCompareTo(t2) < 0 );
	}
	size_t operator()( Term* t ) const{
		return t->hashCode();
	}
};

CL_NS_END
#endif
