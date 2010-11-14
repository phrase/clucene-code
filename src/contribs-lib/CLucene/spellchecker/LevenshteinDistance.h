#ifndef _if_levenshteindistance_header_
#define _if_levenshteindistance_header_

#include "ApiDef.h"
#include "StringDistance.h"

/**
 * Calculates the distance between two words by using the Levenshtein string
 * distance algorithm.
 */
class CLUCENE_CONTRIBS_EXPORT LevenshteinDistanceC : public StringDistanceC
{
public:
  LevenshteinDistanceC();
  float getDistance(const TCHAR *s1, const TCHAR *s2) const;
};

#endif
