/**
 * Copyright 2002-2004 The Apache Software Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _lucene_search_highlight_weightedspanterm_
#define _lucene_search_highlight_weightedspanterm_

#include "WeightedTerm.h"

CL_NS_DEF2(search,highlight)

/**
 * Lightweight class to hold term, weight, and positions used for scoring this
 * term.
 */
class CLUCENE_CONTRIBS_EXPORT WeightedSpanTerm : public WeightedTerm
{
public:
    class PositionSpan : LUCENE_REFBASE 
    {
    public:
        int32_t start;
        int32_t end;

        PositionSpan( int32_t _start, int32_t _end )
        {
            start = _start;
            end = _end;
        }

        virtual ~PositionSpan() {}
    };

protected:
    bool                      m_bPositionSensitive;
    vector<PositionSpan *>    m_vPositionSpans;

public:
    /**
     * @param weight
     * @param term
     */
    WeightedSpanTerm( float_t weight, const TCHAR * term );

    /**
     * @param weight
     * @param term
     * @param positionSensitive
     */
    WeightedSpanTerm( float_t weight, const TCHAR * term, bool bPositionSensitive );

    /// Destructor
    virtual ~WeightedSpanTerm();

    /**
     * Checks to see if this term is valid at <code>position</code>.
     * @param position to check against valid term postions
     * @return true iff this term is a hit at this position
     */
    bool checkPosition( int32_t position );

    void addPositionSpans( const vector<PositionSpan *>& vPositions );
    bool isPositionSensitive();
    void setPositionSensitive( bool bPositionSensitive );
    vector<PositionSpan *>& getPositionSpans();
};

CL_NS_END2

#endif // _lucene_search_highlight_weightedspanterm_
