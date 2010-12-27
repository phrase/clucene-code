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

#include "CLucene/_ApiHeader.h"
#include "WeightedSpanTerm.h"

CL_NS_DEF2(search,highlight)

WeightedSpanTerm::WeightedSpanTerm( float_t weight, const TCHAR * term )
: WeightedTerm( weight, term )
{
    m_bPositionSensitive = false;
}

WeightedSpanTerm::WeightedSpanTerm( float_t weight, const TCHAR * term, bool bPositionSensitive )
: WeightedTerm( weight, term )
{
    m_bPositionSensitive = bPositionSensitive;
}

WeightedSpanTerm::~WeightedSpanTerm()
{
    for( vector<PositionSpan *>::iterator iPS = m_vPositionSpans.begin(); iPS != m_vPositionSpans.end(); iPS++ )
        _CLLDECDELETE( *iPS );

    m_vPositionSpans.clear();
}

bool WeightedSpanTerm::checkPosition( int32_t position )
{
    for( vector<PositionSpan *>::iterator iPS = m_vPositionSpans.begin(); iPS != m_vPositionSpans.end(); iPS++ )
    {
        if(( position >= (*iPS)->start ) && ( position <= (*iPS)->end ))
            return true;
    }
    return false;
}

void WeightedSpanTerm::addPositionSpans( const vector<PositionSpan *>& vPositions )
{
    if( vPositions.size() > 0 )
    {
        m_vPositionSpans.reserve( m_vPositionSpans.size() + vPositions.size() + 1 );
        for( vector<PositionSpan *>::const_iterator iPS = vPositions.begin(); iPS != vPositions.end(); iPS++ )
        {
            PositionSpan * pS = *iPS;
            m_vPositionSpans.push_back( _CL_POINTER( pS ));
        }
    }
}

bool WeightedSpanTerm::isPositionSensitive()
{
    return m_bPositionSensitive;
}

void WeightedSpanTerm::setPositionSensitive( bool bPositionSensitive )
{
    m_bPositionSensitive = bPositionSensitive;
}

vector<WeightedSpanTerm::PositionSpan *>& WeightedSpanTerm::getPositionSpans()
{
    return m_vPositionSpans;
}

CL_NS_END2
