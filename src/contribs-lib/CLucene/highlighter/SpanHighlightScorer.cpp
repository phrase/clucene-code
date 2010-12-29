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

SpanHighlightScorer::SpanHighlightScorer( CL_NS(search)::Query * query, const TCHAR * field, CL_NS(analysis)::TokenStream * tokenStream, bool autoRewriteQueries, WeightedSpanTermExtractor * customExtractor )
{
    this->autoRewriteQueries        = autoRewriteQueries;
    this->totalScore                = 0;
    this->maxTermWeight             = 0;
    this->position                  = -1;
    this->deleteWeightedSpanTerms   = true;

    init( query, field, tokenStream, NULL, customExtractor );
}

SpanHighlightScorer::SpanHighlightScorer( CL_NS(search)::Query * query, const TCHAR * field, CL_NS(analysis)::TokenStream * tokenStream, CL_NS(index)::IndexReader * reader, bool autoRewriteQueries, WeightedSpanTermExtractor * customExtractor )
{
    this->autoRewriteQueries        = autoRewriteQueries;
    this->totalScore                = 0;
    this->maxTermWeight             = 0;
    this->position                  = -1;
    this->deleteWeightedSpanTerms   = true;

    init( query, field, tokenStream, reader, customExtractor );
}

SpanHighlightScorer::SpanHighlightScorer( WeightedSpanTerm ** weightedTerms, size_t nCount, bool deleteTerms, bool autoRewriteQueries )
{
    this->autoRewriteQueries        = autoRewriteQueries;
    this->totalScore                = 0;
    this->maxTermWeight             = 0;
    this->position                  = -1;
    this->deleteWeightedSpanTerms   = deleteTerms;

    for( size_t i = 0; i < nCount; i++ )
    {
        if( maxTermWeight < weightedTerms[ i ]->getWeight())
            maxTermWeight = weightedTerms[ i ]->getWeight();

        WeightedSpanTermMap::iterator iTerm = fieldWeightedSpanTerms.find( weightedTerms[ i ]->getTerm() );
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
        for( WeightedSpanTermMap::iterator iST = fieldWeightedSpanTerms.begin(); iST != fieldWeightedSpanTerms.end(); iST++ )
            _CLDELETE( iST->second );
    }
    fieldWeightedSpanTerms.clear();
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

    WeightedSpanTermMap::iterator iTerm = fieldWeightedSpanTerms.find( termText );
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
    WeightedSpanTermMap::iterator iTerm = fieldWeightedSpanTerms.find( tszToken );
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

bool SpanHighlightScorer::isAutoRewritingQueries()
{
    return autoRewriteQueries;
}

void SpanHighlightScorer::init( CL_NS(search)::Query * query, const TCHAR * field, CL_NS(analysis)::TokenStream * tokenStream, CL_NS(index)::IndexReader * reader, WeightedSpanTermExtractor * customExtractor )
{
    WeightedSpanTermExtractor * extractor = customExtractor ? customExtractor : _CLNEW WeightedSpanTermExtractor( autoRewriteQueries );
    extractor->setAutoRewriteQueries( autoRewriteQueries );

    if( ! reader ) 
         extractor->getWeightedSpanTerms( fieldWeightedSpanTerms, query, tokenStream, field );
    else 
         extractor->getWeightedSpanTermsWithScores( fieldWeightedSpanTerms, query, tokenStream, field, reader );

    if( ! customExtractor )
        _CLLDELETE( extractor );
}

CL_NS_END2