/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/

CL_NS_DEF(util)

template<typename _target, typename _source>
std::auto_ptr<_target> auto_ptr_static_cast(std::auto_ptr<_source> in) {
	std::auto_ptr<_target> result(static_cast<_target*>(in.release()));
	return result;
}

CL_NS_END

