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

#include "CLucene/analysis/AnalysisHeader.h"

#include "SimpleSpanFragmenter.h"
#include "WeightedSpanTerm.h"
#include "SpanHighlightScorer.h"

CL_NS_DEF2(search,highlight)
CL_NS_USE(analysis)


SimpleSpanFragmenter::SimpleSpanFragmenter( SpanHighlightScorer * pSpanScorer )
{
    m_nFragmentSize    = DEFAULT_FRAGMENT_SIZE;
    m_nCurrentNumFrags = 0;
    m_nPosition        = -1;
    m_nWaitForPos      = -1;
    m_pSpanScorer      = pSpanScorer;
}

SimpleSpanFragmenter::SimpleSpanFragmenter( SpanHighlightScorer * pSpanScorer, int32_t nFragmentSize )
{
    m_nFragmentSize    = nFragmentSize;
    m_nCurrentNumFrags = 0;
    m_nPosition        = -1;
    m_nWaitForPos      = -1;
    m_pSpanScorer      = pSpanScorer;
}

SimpleSpanFragmenter::~SimpleSpanFragmenter()
{
}

bool SimpleSpanFragmenter::isNewFragment( const CL_NS(analysis)::Token * pToken )
{
    _ASSERT( pToken );
    m_nPosition += pToken->getPositionIncrement();

    if( m_nWaitForPos == m_nPosition )
        m_nWaitForPos = -1;
    else if( m_nWaitForPos != -1 )
        return false;

    WeightedSpanTerm * pSpanTerm = m_pSpanScorer->getWeightedSpanTerm( pToken->termBuffer() );
    if( pSpanTerm )
    {
        vector<WeightedSpanTerm::PositionSpan *>& vPositionSpans = pSpanTerm->getPositionSpans();
        for( vector<WeightedSpanTerm::PositionSpan *>::iterator iPS = vPositionSpans.begin(); iPS != vPositionSpans.end(); iPS++ )
        {
            if( (*iPS)->start == m_nPosition ) 
            {
                m_nWaitForPos = (*iPS)->end + 1;
                return true;
            }
        }
    }

    bool isNewFrag = pToken->endOffset() >= ( m_nFragmentSize * m_nCurrentNumFrags );
    if( isNewFrag )
        m_nCurrentNumFrags++;

    return isNewFrag;
}

void SimpleSpanFragmenter::start( const TCHAR* originalText )
{
    m_nPosition = 0;
    m_nCurrentNumFrags = 1;
}

CL_NS_END2