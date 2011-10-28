/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/

#ifndef _lucene_workaround_vc6_DocumentsWriter_
#define _lucene_workaround_vc6_DocumentsWriter_

#include <iostream>

std::ostream& operator << (std::ostream& os, __int64 i);

#endif
