/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include <boost/shared_ptr.hpp>
#include "Term.h"
#include "Terms.h"

CL_NS_DEF(index)

Term::Pointer TermEnum::term(bool pointer){
	return term();
}

TermEnum::~TermEnum(){
}

bool TermEnum::skipTo(const Term::Pointer& target){
	do {
		if (!next())
			return false;
	} while (target->compareTo(term(false)) > 0);
	return true;
}

TermPositions::~TermPositions(){
}

CL_NS_END
