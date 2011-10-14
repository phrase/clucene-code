/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/

#include "operators.h"

// See also http://support.microsoft.com/kb/168440

std::ostream& operator<<(std::ostream& os, __int64 i )
{  
    char buf[20];
    sprintf(buf,"%I64d", i );
    os << buf;
    return os;
}
