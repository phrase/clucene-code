#ifndef _if_stringdistance_header_
#define _if_stringdistance_header_

#include "ApiDef.h"

/**
 * Abstract base class of all implementation, which calculates the
 * distance between two strings.
 */
class CLUCENE_CONTRIBS_EXPORT StringDistanceC
{
public:
  virtual float getDistance(const TCHAR *s1, const TCHAR *s2) const = 0;
};

// Declare some global functions.
int CLUCENE_CONTRIBS_EXPORT minimum( int a, int b );
int CLUCENE_CONTRIBS_EXPORT maximum( int a, int b );

#endif
