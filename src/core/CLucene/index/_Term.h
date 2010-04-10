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

CL_NS_DEF(index)


class Term_Equals:public CL_NS_STD(binary_function)<Term::ConstPointer, Term::ConstPointer, bool>
{
public:
	bool operator() (Term::ConstPointer val1, Term::ConstPointer val2) const {
		return val1->equals(val2);
	}
};

class Term_Compare:LUCENE_BASE, public CL_NS(util)::Compare::_base //<Term*>
{
public:
	bool operator() (Term::Pointer t1, Term::Pointer t2) const {
		return ( t1->compareTo(t2) < 0 );
	}
	size_t operator() (Term::Pointer t) const {
		return t->hashCode();
	}
};

class Term_UnorderedCompare:LUCENE_BASE, public CL_NS(util)::Compare::_base //<Term*>
{
public:
	bool operator() (Term::Pointer t1, Term::Pointer t2) const {
		return ( t1->hashedCompareTo(t2) < 0 );
	}
	size_t operator() (Term::Pointer t) const {
		return t->hashCode();
	}
};

CL_NS_END
#endif
