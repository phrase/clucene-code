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

#include "WeightedSpanTerm.h"
#include "WeightedSpanTermExtractor.h"
#include "SpanHighlightScorer.h"


CL_NS_DEF2(search,highlight)

SpanHighlightScorer::SpanHighlightScorer( bool bAutoRewriteQueries, WeightedSpanTermExtractor * pCustomExtractor, bool bDeleteExtractor )
{
    m_bAutoRewriteQueries       = bAutoRewriteQueries;
    m_pSpanExtractor            = pCustomExtractor;
    m_bDeleteExtractor          = bDeleteExtractor;

    m_bScoreTerms               = false;
    m_pReader                   = NULL;
    m_nDocId                    = -1;

    m_fTotalScore               = 0;
    m_fMaxTermWeight            = 0;
    m_nPosition                 = -1;
    m_bDeleteWeightedSpanTerms  = true;
}

SpanHighlightScorer::SpanHighlightScorer( WeightedSpanTerm ** rgWeightedTerms, size_t nCount, bool bDeleteTerms )
{
    m_bAutoRewriteQueries       = false;
    m_pSpanExtractor            = NULL;
    m_bDeleteExtractor          = false;

    m_bScoreTerms               = false;
    m_pReader                   = NULL;
    m_nDocId                    = -1;

    m_fTotalScore               = 0;
    m_fMaxTermWeight            = 0;
    m_nPosition                 = -1;
    m_bDeleteWeightedSpanTerms  = bDeleteTerms;

    for( size_t i = 0; i < nCount; i++ )
    {
        if( m_fMaxTermWeight < rgWeightedTerms[ i ]->getWeight())
            m_fMaxTermWeight = rgWeightedTerms[ i ]->getWeight();

        WeightedSpanTermMap::iterator iTerm = m_fieldWeightedSpanTerms.find( rgWeightedTerms[ i ]->getTerm() );
        if( iTerm == m_fieldWeightedSpanTerms.end() )
        {
            m_fieldWeightedSpanTerms[ rgWeightedTerms[ i ]->getTerm() ] = rgWeightedTerms[ i ];
        } 
        else if( iTerm->second->getWeight() < rgWeightedTerms[ i ]->getWeight() ) 
        {
            // if a term is defined more than once, always use the highest scoring weight
            iTerm->second->setWeight( rgWeightedTerms[ i ]->getWeight() );
            if( bDeleteTerms )
                _CLLDELETE( rgWeightedTerms[ i ] );
        }
    }
}

SpanHighlightScorer::~SpanHighlightScorer()
{
    freeWeightedSpanTerms();

    if( m_bDeleteExtractor )
        delete m_pSpanExtractor;
    m_pSpanExtractor = NULL;
}

void SpanHighlightScorer::freeWeightedSpanTerms()
{
    if( m_bDeleteWeightedSpanTerms )
    {
        for( WeightedSpanTermMap::iterator iST = m_fieldWeightedSpanTerms.begin(); iST != m_fieldWeightedSpanTerms.end(); iST++ )
            _CLDELETE( iST->second );
    }
    m_fieldWeightedSpanTerms.clear();
}

void SpanHighlightScorer::setIndexReader( CL_NS(index)::IndexReader * pReader )
{
    m_pReader = pReader;
}

CL_NS(index)::IndexReader * SpanHighlightScorer::getIndexReader()
{
    return m_pReader;
}

void SpanHighlightScorer::setScoreTerms( bool bScoreTerms )
{
    m_bScoreTerms = bScoreTerms;
}

bool SpanHighlightScorer::scoreTerms()
{
    return m_bScoreTerms;
}

void SpanHighlightScorer::setAutoRewriteQueries( bool bAutoRewriteQueries )
{
    m_bAutoRewriteQueries = bAutoRewriteQueries;
}

bool SpanHighlightScorer::autoRewriteQueries()
{
    return m_bAutoRewriteQueries;
}

void SpanHighlightScorer::init( CL_NS(search)::Query * pQuery, const TCHAR * tszField, CL_NS(analysis)::TokenStream * pTokenStream, int32_t nDocId )
{
    if( ! m_pSpanExtractor )
    {
        m_pSpanExtractor   = _CLNEW WeightedSpanTermExtractor();
        m_bDeleteExtractor = true;
    }
             
    m_pSpanExtractor->setAutoRewriteQueries( m_bAutoRewriteQueries );
    m_pSpanExtractor->setScoreTerms( m_bScoreTerms );
    m_pSpanExtractor->setIndexReader( m_pReader );

    freeWeightedSpanTerms();
    m_bDeleteWeightedSpanTerms = true;
    m_pSpanExtractor->extractWeightedSpanTerms( m_fieldWeightedSpanTerms, pQuery, tszField, pTokenStream, nDocId );
}

float_t SpanHighlightScorer::getFragmentScore()
{
    return m_fTotalScore;
}

float_t SpanHighlightScorer::getMaxTermWeight()
{
    return m_fMaxTermWeight;
}

float_t SpanHighlightScorer::getTokenScore( CL_NS(analysis)::Token * pToken )
{
    m_nPosition += pToken->getPositionIncrement();
    const TCHAR * tszTermText = pToken->termText();

    WeightedSpanTermMap::iterator iTerm = m_fieldWeightedSpanTerms.find( tszTermText );
    if( iTerm == m_fieldWeightedSpanTerms.end() )
        return 0;

    WeightedSpanTerm * pWeightedSpanTerm = iTerm->second;
    if( pWeightedSpanTerm->isPositionSensitive() && ! pWeightedSpanTerm->checkPosition( m_nPosition ))
        return 0;

    float_t fScore = pWeightedSpanTerm->getWeight();

    // found a query term - is it unique in this doc?
    if( m_foundTerms.find( tszTermText ) == m_foundTerms.end() )
    {
        m_fTotalScore += fScore;
        m_foundTerms.insert( tszTermText );
    }

    return fScore;
}

WeightedSpanTerm * SpanHighlightScorer::getWeightedSpanTerm( const TCHAR * tszToken )
{
    WeightedSpanTermMap::iterator iTerm = m_fieldWeightedSpanTerms.find( tszToken );
    if( iTerm == m_fieldWeightedSpanTerms.end() )
        return NULL;
    else 
        return iTerm->second;
}

void SpanHighlightScorer::reset()
{
    m_nPosition = -1;
}

void SpanHighlightScorer::startFragment( TextFragment * newFragment )
{
    m_foundTerms.clear();
    m_fTotalScore = 0;
}

CL_NS_END2