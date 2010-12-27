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

SpanHighlightScorer::SpanHighlightScorer( CL_NS(search)::Query * query, const TCHAR * field, CL_NS(analysis)::CachingTokenFilter * cachingTokenFilter, bool hilitCnstScrRngQuery )
{
    highlightCnstScrRngQuery = hilitCnstScrRngQuery;
    maxTermWeight = 0;
    position = -1;
    defaultField = NULL;
    deleteWeightedSpanTerms = true;
    init( query, field, cachingTokenFilter, NULL );
}

SpanHighlightScorer::SpanHighlightScorer( CL_NS(search)::Query * query, const TCHAR * field, CL_NS(analysis)::CachingTokenFilter * cachingTokenFilter, CL_NS(index)::IndexReader * reader, bool hilitCnstScrRngQuery )
{
    highlightCnstScrRngQuery = hilitCnstScrRngQuery;
    maxTermWeight = 0;
    position = -1;
    defaultField = NULL;
    deleteWeightedSpanTerms = true;
    init( query, field, cachingTokenFilter, reader );
}

SpanHighlightScorer::SpanHighlightScorer( CL_NS(search)::Query * query, const TCHAR * field, CL_NS(analysis)::CachingTokenFilter * cachingTokenFilter, CL_NS(index)::IndexReader * reader, const TCHAR * _defaultField, bool hilitCnstScrRngQuery )
{
    highlightCnstScrRngQuery = hilitCnstScrRngQuery;
    maxTermWeight = 0;
    position = -1;
    defaultField = _defaultField ? STRDUP_TtoT( _defaultField ) : NULL;
    deleteWeightedSpanTerms = true;
    init( query, field, cachingTokenFilter, reader );
}

SpanHighlightScorer::SpanHighlightScorer( CL_NS(search)::Query * query, const TCHAR * field, CL_NS(analysis)::CachingTokenFilter * cachingTokenFilter, const TCHAR * _defaultField, bool hilitCnstScrRngQuery )
{
    highlightCnstScrRngQuery = hilitCnstScrRngQuery;
    maxTermWeight = 0;
    position = -1;
    defaultField = _defaultField ? STRDUP_TtoT( _defaultField ) : NULL;
    deleteWeightedSpanTerms = true;
    init( query, field, cachingTokenFilter, NULL );
}

SpanHighlightScorer::SpanHighlightScorer( WeightedSpanTerm ** weightedTerms, size_t nCount, bool deleteTerms, bool hilitCnstScrRngQuery )
{
    highlightCnstScrRngQuery = hilitCnstScrRngQuery;
    maxTermWeight = 0;
    position = -1;
    defaultField = NULL;
    fieldWeightedSpanTerms.clear();
    deleteWeightedSpanTerms = deleteTerms;

    for( size_t i = 0; i < nCount; i++ )
    {
        if( maxTermWeight < weightedTerms[ i ]->getWeight())
            maxTermWeight = weightedTerms[ i ]->getWeight();

        map<tstring, WeightedSpanTerm *>::iterator iTerm = fieldWeightedSpanTerms.find( weightedTerms[ i ]->getTerm() );
        if( iTerm == fieldWeightedSpanTerms.end() )
        {
            fieldWeightedSpanTerms[ weightedTerms[ i ]->getTerm() ] = weightedTerms[ i ];
        } 
        else if( iTerm->second->getWeight() < weightedTerms[ i ]->getWeight() ) 
        {
            // if a term is defined more than once, always use the highest scoring weight
            iTerm->second->setWeight( weightedTerms[ i ]->getWeight() );
            if( deleteTerms )
                _CLLDELETE( weightedTerms[ i ] );
        }
    }
}

SpanHighlightScorer::~SpanHighlightScorer()
{
    if( deleteWeightedSpanTerms )
    {
        for( map<tstring, WeightedSpanTerm *>::iterator iST = fieldWeightedSpanTerms.begin(); iST != fieldWeightedSpanTerms.end(); iST++ )
            _CLDELETE( iST->second );
    }
    fieldWeightedSpanTerms.clear();

    _CLDELETE_ARRAY( defaultField );
}

float_t SpanHighlightScorer::getFragmentScore()
{
    return totalScore;
}

float_t SpanHighlightScorer::getMaxTermWeight()
{
    return maxTermWeight;
}

float_t SpanHighlightScorer::getTokenScore( CL_NS(analysis)::Token * pToken )
{
    position += pToken->getPositionIncrement();
    const TCHAR * termText = pToken->termText();

    map<tstring, WeightedSpanTerm *>::iterator iTerm = fieldWeightedSpanTerms.find( termText );
    if( iTerm == fieldWeightedSpanTerms.end() )
        return 0;

    WeightedSpanTerm * weightedSpanTerm = iTerm->second;
    if( weightedSpanTerm->isPositionSensitive() && ! weightedSpanTerm->checkPosition( position ))
        return 0;

    float_t score = weightedSpanTerm->getWeight();

    // found a query term - is it unique in this doc?
    if( foundTerms.find( termText ) == foundTerms.end() )
    {
        totalScore += score;
        foundTerms.insert( termText );
    }

    return score;
}

WeightedSpanTerm * SpanHighlightScorer::getWeightedSpanTerm( const TCHAR * tszToken )
{
    map<tstring, WeightedSpanTerm *>::iterator iTerm = fieldWeightedSpanTerms.find( tszToken );
    if( iTerm == fieldWeightedSpanTerms.end() )
        return NULL;
    else 
        return iTerm->second;
}

void SpanHighlightScorer::reset()
{
    position = -1;
}

void SpanHighlightScorer::startFragment( TextFragment * newFragment )
{
    foundTerms.clear();
    totalScore = 0;
}

bool SpanHighlightScorer::isHighlightCnstScrRngQuery()
{
    return highlightCnstScrRngQuery;
}

void SpanHighlightScorer::setHighlightCnstScrRngQuery( bool highlight )
{
    highlightCnstScrRngQuery = highlight;
}

void SpanHighlightScorer::init( CL_NS(search)::Query * query, const TCHAR * field, CL_NS(analysis)::CachingTokenFilter * cachingTokenFilter, CL_NS(index)::IndexReader * reader )
{
    WeightedSpanTermExtractor * qse = ( defaultField ) ? _CLNEW WeightedSpanTermExtractor( defaultField ) : _CLNEW WeightedSpanTermExtractor();
    
    qse->setHighlightCnstScrRngQuery(highlightCnstScrRngQuery);
    if( ! reader ) 
         qse->getWeightedSpanTerms( fieldWeightedSpanTerms, query, cachingTokenFilter, field );
    else 
         qse->getWeightedSpanTermsWithScores( fieldWeightedSpanTerms, query, cachingTokenFilter, field, reader );
    _CLDELETE( qse );    
}

CL_NS_END2