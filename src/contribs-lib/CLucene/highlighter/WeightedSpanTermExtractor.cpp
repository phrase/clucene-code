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

#include "CLucene/index/IndexReader.h"
#include "CLucene/index/IndexWriter.h"
#include "CLucene/index/Terms.h"
#include "CLucene/index/Term.h"

#include "CLucene/document/Document.h"

#include "CLucene/analysis/Analyzers.h"
// #include "CLucene/analysis/CachingTokenFilter.h"

#include "CLucene/search/Query.h"
#include "CLucene/search/BooleanQuery.h"
#include "CLucene/search/BooleanClause.h"
#include "CLucene/search/PhraseQuery.h"
#include "CLucene/search/TermQuery.h"
#include "CLucene/search/MultiPhraseQuery.h"
#include "CLucene/search/ConstantScoreQuery.h"
#include "CLucene/search/FuzzyQuery.h"
#include "CLucene/search/WildcardQuery.h"
#include "CLucene/search/PrefixQuery.h"
#include "CLucene/search/RangeQuery.h"
#include "CLucene/search/MatchAllDocsQuery.h"

#include "CLucene/search/spans/Spans.h"
#include "CLucene/search/spans/SpanTermQuery.h"
#include "CLucene/search/spans/SpanNearQuery.h"
#include "CLucene/search/spans/SpanNotQuery.h"
#include "CLucene/search/spans/SpanFirstQuery.h"
#include "CLucene/search/spans/SpanOrQuery.h"

#include "CLucene/store/RAMDirectory.h"

#include "CLucene/util/_Arrays.h"

#include "WeightedSpanTermExtractor.h"
#include "WeightedSpanTerm.h"


CL_NS_USE(index);
CL_NS_USE(analysis);
CL_NS_USE(document);
CL_NS_USE(search);
CL_NS_USE2(search,spans);
CL_NS_USE(store);
CL_NS_USE(util);


CL_NS_DEF2(search,highlight)


/////////////////////////////////////////////////////////////////////////////
WeightedSpanTermExtractor::PositionCheckingMap::PositionCheckingMap( WeightedSpanTermMap& weightedSpanTerms )
: mapSpanTerms( weightedSpanTerms )
{
}

WeightedSpanTermExtractor::PositionCheckingMap::~PositionCheckingMap()
{
}

void WeightedSpanTermExtractor::PositionCheckingMap::putAll( WeightedSpanTermExtractor::PositionCheckingMap * m )
{
    for( WeightedSpanTermMap::iterator iST = m->mapSpanTerms.begin(); iST != m->mapSpanTerms.end(); iST++ )
        put( iST->second );
}

void WeightedSpanTermExtractor::PositionCheckingMap::put( WeightedSpanTerm * spanTerm )
{
    const TCHAR * term = spanTerm->getTerm();
    WeightedSpanTermMap::iterator iST = mapSpanTerms.find( term );
    if( iST != mapSpanTerms.end() && ! iST->second->isPositionSensitive() )
        spanTerm->setPositionSensitive( false );
    mapSpanTerms[ term ] = spanTerm;
}

WeightedSpanTerm * WeightedSpanTermExtractor::PositionCheckingMap::get( const TCHAR * term )
{
    WeightedSpanTermMap::iterator iST = mapSpanTerms.find( term );
    if( iST != mapSpanTerms.end() )
        return iST->second;
    else
        return NULL;
}


/////////////////////////////////////////////////////////////////////////////
WeightedSpanTermExtractor::WeightedSpanTermExtractor( bool bAutoRewriteQueries )
{
    m_bAutoRewriteQueries    = bAutoRewriteQueries;
    
    m_tszFieldName           = NULL;
    m_pTokenStream           = NULL;
    m_pReader                = NULL;
    m_pFieldReader           = NULL;
    m_nDocId                 = -1;

    m_bScoreTerms            = false;
}

WeightedSpanTermExtractor::~WeightedSpanTermExtractor()
{
    closeFieldReader();
}

void WeightedSpanTermExtractor::setIndexReader( CL_NS(index)::IndexReader * pReader )
{
    m_pReader = pReader;
}

void WeightedSpanTermExtractor::setScoreTerms( bool bScoreTerms )
{
    m_bScoreTerms = bScoreTerms;
}

bool WeightedSpanTermExtractor::scoreTerms()
{
    return m_bScoreTerms;
}

void WeightedSpanTermExtractor::setAutoRewriteQueries( bool bAutoRewriteQueries )
{
    m_bAutoRewriteQueries = bAutoRewriteQueries;
}

bool WeightedSpanTermExtractor::autoRewriteQueries()
{
    return m_bAutoRewriteQueries;
}

void WeightedSpanTermExtractor::extractWeightedSpanTerms( WeightedSpanTermMap& weightedSpanTerms, CL_NS(search)::Query * pQuery, const TCHAR * tszFieldName, CL_NS(analysis)::TokenStream * pTokenStream, int32_t nDocId )
{
    m_tszFieldName = tszFieldName;
    m_pTokenStream = pTokenStream;
    m_nDocId       = nDocId;

    WeightedSpanTermExtractor::PositionCheckingMap terms( weightedSpanTerms );
    extract( pQuery, terms );
    
    closeFieldReader();
    
    m_tszFieldName = NULL;
    m_pTokenStream = NULL;
    m_nDocId       = -1;

    if( m_bScoreTerms && m_pReader )
    {
        int32_t nTotalNumDocs = m_pReader->numDocs();
        Term *  pTerm = _CLNEW Term();
        for( WeightedSpanTermMap::iterator iSpanTerm = weightedSpanTerms.begin(); iSpanTerm != weightedSpanTerms.end(); iSpanTerm++ )
        {
            pTerm->set( tszFieldName, iSpanTerm->first );
            int32_t nDocFreq = m_pReader->docFreq( pTerm );

            if( nTotalNumDocs < nDocFreq )
                nDocFreq = nTotalNumDocs;

            // IDF algorithm taken from DefaultSimilarity class
            float_t idf = (float_t)( log( (float_t)nTotalNumDocs / (float_t)(nDocFreq + 1) ) + 1.0f );
            iSpanTerm->second->setWeight( iSpanTerm->second->getWeight() * idf );
        }
    }
}

bool WeightedSpanTermExtractor::matchesField( const TCHAR * tszFieldNameToCheck )
{
    return ( tszFieldNameToCheck == m_tszFieldName || 0 == _tcscmp( tszFieldNameToCheck, m_tszFieldName ));
}

void WeightedSpanTermExtractor::extract( Query * pQuery, WeightedSpanTermExtractor::PositionCheckingMap& terms )
{
    if( pQuery->instanceOf( TermQuery::getClassName() ))
    {
        extractWeightedTerms( pQuery, terms );
    }
    else if( pQuery->instanceOf( BooleanQuery::getClassName() ))
    {
        extractFromBooleanQuery( (BooleanQuery *) pQuery, terms );
    }
    else if( pQuery->instanceOf( PhraseQuery::getClassName() )) 
    {
        extractFromPhraseQuery( (PhraseQuery *) pQuery, terms );
    }
    else if( pQuery->instanceOf( SpanFirstQuery::getClassName() )
          || pQuery->instanceOf( SpanNearQuery::getClassName() )
          || pQuery->instanceOf( SpanNotQuery::getClassName() )
          || pQuery->instanceOf( SpanOrQuery::getClassName() )
          || pQuery->instanceOf( SpanTermQuery::getClassName() ))
    {
        extractWeightedSpanTerms( (SpanQuery *) pQuery, terms );
    } 
    else if( pQuery->instanceOf( MultiPhraseQuery::getClassName() )) 
    {
        extractFromMultiPhraseQuery( (MultiPhraseQuery *) pQuery, terms );
    }
    else if( pQuery->instanceOf( ConstantScoreRangeQuery::getClassName() ))
    {
        extractFromConstantScoreRangeQuery( (ConstantScoreRangeQuery *) pQuery, terms );
    }
    else if( pQuery->instanceOf( FuzzyQuery::getClassName() )
          || pQuery->instanceOf( PrefixQuery::getClassName() )
          || pQuery->instanceOf( RangeQuery::getClassName() )
          || pQuery->instanceOf( WildcardQuery::getClassName() ))
    { 
        rewriteAndExtract( pQuery, terms );
    }
    else if( ! pQuery->instanceOf( MatchAllDocsQuery::getClassName() )
          && ! pQuery->instanceOf( ConstantScoreQuery::getClassName() ))
    
    {
        extractFromCustomQuery( pQuery, terms );
    }
}

void WeightedSpanTermExtractor::extractFromBooleanQuery( BooleanQuery * pQuery, WeightedSpanTermExtractor::PositionCheckingMap& terms )
{
    int32_t nClauses = pQuery->getClauseCount();
    if( nClauses > 0 )
    {
        BooleanClause ** rgQueryClauses = _CL_NEWARRAY( BooleanClause*, nClauses );
        pQuery->getClauses( rgQueryClauses );

        for( int32_t i = 0; i < nClauses; i++ )
        {
            if( ! rgQueryClauses[ i ]->isProhibited() )
                extract( rgQueryClauses[ i ]->getQuery(), terms );
        }
        _CLDELETE_ARRAY( rgQueryClauses );
    }
} 

void WeightedSpanTermExtractor::extractFromPhraseQuery( PhraseQuery * pQuery, WeightedSpanTermExtractor::PositionCheckingMap& terms )
{
    Term** rgPhraseQueryTerms = pQuery->getTerms();
    vector<SpanQuery *> vSpans;
    for( int32_t i = 0; rgPhraseQueryTerms[ i ]; i++ )
        vSpans.push_back( _CLNEW SpanTermQuery( rgPhraseQueryTerms[ i ] ));

    int32_t nSlop = pQuery->getSlop();
    bool bInOrder = ( nSlop == 0 );

    SpanNearQuery * pNearQuery = _CLNEW SpanNearQuery( vSpans.begin(), vSpans.end(), nSlop, bInOrder, true );
    pNearQuery->setBoost( pQuery->getBoost() );
    
    extractWeightedSpanTerms( pNearQuery, terms );

    _CLDELETE( pNearQuery );
    _CLDELETE_ARRAY( rgPhraseQueryTerms );
} 

void WeightedSpanTermExtractor::extractFromMultiPhraseQuery( MultiPhraseQuery * pQuery, WeightedSpanTermExtractor::PositionCheckingMap& terms )
{
    const CLArrayList<ArrayBase<Term*>*>* termArrays   = NULL;
    ValueArray<int32_t>                   positions;

    termArrays = pQuery->getTermArrays();
    pQuery->getPositions( positions );

    if( positions.length > 0 )
    {
        int32_t nMaxPosition = positions[ positions.length - 1 ];
        for( size_t i = 0; i < positions.length - 1; i++ )
        {
            if( positions[ i ] > nMaxPosition )
                nMaxPosition = positions[ i ];
        }

        vector<SpanTermQuery *>** disjunctLists = _CL_NEWARRAY( vector<SpanTermQuery *>*, nMaxPosition + 1 );
        memset( disjunctLists, 0, sizeof( vector<SpanTermQuery *>* ) * ( nMaxPosition + 1 ) );
        int32_t nDistinctPositions = 0;
        for( size_t i = 0; i < termArrays->size(); i++ )
        {
            ArrayBase<Term*> * termArray = termArrays->at( i );
            if( ! disjunctLists[ positions[ i ]] )
                disjunctLists[ positions[ i ]] = _CLNEW vector<SpanTermQuery *>;
            vector<SpanTermQuery *>& disjuncts = *( disjunctLists[ positions[ i ]] );
            if( disjuncts.empty() )
                nDistinctPositions++;
    
            for( size_t j = 0; j < termArray->length; j++ )
                disjuncts.push_back( _CLNEW SpanTermQuery( (*termArray)[ j ] ));
        }

        int32_t nPositionGaps = 0;
        int32_t nPosition = 0;
        SpanQuery** rgClauses = _CL_NEWARRAY( SpanQuery*, nDistinctPositions );
        for( int32_t i = 0; i <= nMaxPosition; i++ )
        {
            vector<SpanTermQuery *> * pDisjuncts = disjunctLists[ i ];
            if( pDisjuncts && ! pDisjuncts->empty() )
                rgClauses[ nPosition++ ] = _CLNEW SpanOrQuery( pDisjuncts->begin(), pDisjuncts->end(), true );
            else
                nPositionGaps++;
        }

        int32_t nSlop = pQuery->getSlop();
        bool bInOrder = ( nSlop == 0 );

        SpanNearQuery * pNearQuery = _CLNEW SpanNearQuery( rgClauses, rgClauses + nDistinctPositions, nSlop + nPositionGaps, bInOrder, true );
        pNearQuery->setBoost( pQuery->getBoost() );
        extractWeightedSpanTerms( pNearQuery, terms );

        _CLDELETE( pNearQuery );
        _CLDELETE_ARRAY( rgClauses );
        for( int32_t i = 0; i <= nMaxPosition; i++ )
            _CLDELETE( disjunctLists[ i ] );
        _CLDELETE_ARRAY( disjunctLists ); 
    }
} 

void WeightedSpanTermExtractor::extractFromConstantScoreRangeQuery( ConstantScoreRangeQuery * pQuery, WeightedSpanTermExtractor::PositionCheckingMap& terms )
{
    if( m_bAutoRewriteQueries && matchesField( pQuery->getField() ))
    {
        Term * pLower = _CLNEW Term( m_tszFieldName, pQuery->getLowerVal() );
        Term * pUpper = _CLNEW Term( m_tszFieldName, pQuery->getUpperVal() );

        IndexReader * pReader = getFieldReader();
        try 
        {
            TermEnum * pTermEnum = pReader->terms( pLower );
            TermSet setTerms;
            do 
            {
                Term * pTerm = pTermEnum->term( true );
                if( pTerm && pUpper->compareTo( pTerm ) >= 0 )
                    setTerms.insert( pTerm );
                else
                {
                    _CLLDECDELETE( pTerm );
                    break;
                }
            }
            while( pTermEnum->next() );

            processNonWeightedTerms( terms, setTerms, pQuery->getBoost() );
            clearTermSet( setTerms );
            _CLLDELETE( pTermEnum );
        }
        catch( ... )
        {
            _CLLDECDELETE( pUpper );
            _CLLDECDELETE( pLower );
            throw;
        }
        _CLLDECDELETE( pUpper );
        _CLLDECDELETE( pLower );
    }
}

void WeightedSpanTermExtractor::rewriteAndExtract( Query * pQuery, WeightedSpanTermExtractor::PositionCheckingMap& terms )
{
    if( m_bAutoRewriteQueries )
    {
        IndexReader * pReader = getFieldReader();
        Query * pRewrittenQuery = pQuery->rewrite( pReader );
        if( pRewrittenQuery != pQuery )
        {
            try
            {
                extract( pRewrittenQuery, terms );
                _CLLDELETE( pRewrittenQuery );
            }
            catch( ... )
            {
                _CLLDELETE( pRewrittenQuery );
                throw;
            }
        }
    }
}

void WeightedSpanTermExtractor::extractFromCustomQuery( Query * pQuery, WeightedSpanTermExtractor::PositionCheckingMap& terms )
{
}

void WeightedSpanTermExtractor::extractWeightedSpanTerms( CL_NS2(search,spans)::SpanQuery * pSpanQuery, PositionCheckingMap& terms )
{
    vector<WeightedSpanTerm::PositionSpan *>    spanPositions;
    IndexReader *                               pReader = getFieldReader();
    Spans *                                     pSpans  = pSpanQuery->getSpans( pReader );


    if( m_nDocId == -1 )
    {
        while( pSpans->next() )
            spanPositions.push_back( _CLNEW WeightedSpanTerm::PositionSpan( pSpans->start(), pSpans->end() - 1 ));
    }
    else
    {
        if( pSpans->skipTo( m_nDocId ) && pSpans->doc() == m_nDocId  )
        {
            spanPositions.push_back( _CLNEW WeightedSpanTerm::PositionSpan( pSpans->start(), pSpans->end() - 1 ));
            while( pSpans->next() && pSpans->doc() == m_nDocId )
                spanPositions.push_back( _CLNEW WeightedSpanTerm::PositionSpan( pSpans->start(), pSpans->end() - 1 ));
        }
    }
   _CLDELETE( pSpans );

    if( spanPositions.size() != 0 ) 
    {
        TermSet nonWeightedTerms;
        pSpanQuery->extractTerms( &nonWeightedTerms );

        for( TermSet::iterator iter = nonWeightedTerms.begin(); iter != nonWeightedTerms.end(); iter++ )
        {
            Term * pTerm = *iter;
            if( matchesField( pTerm->field() )) 
            {
                WeightedSpanTerm * pWeightedSpanTerm = terms.get( pTerm->text() );
                if( ! pWeightedSpanTerm ) 
                {
                    pWeightedSpanTerm = _CLNEW WeightedSpanTerm( pSpanQuery->getBoost(), pTerm->text() );
                    pWeightedSpanTerm->addPositionSpans( spanPositions );
                    pWeightedSpanTerm->setPositionSensitive( true );
                    terms.put( pWeightedSpanTerm );
                } 
                else 
                {
                    if( spanPositions.size() > 0 )
                        pWeightedSpanTerm->addPositionSpans( spanPositions );
                }
            }
        }

        clearTermSet( nonWeightedTerms );
        for( vector<WeightedSpanTerm::PositionSpan *>::iterator iPS = spanPositions.begin(); iPS != spanPositions.end(); iPS++ )
            _CLLDECDELETE( *iPS );
    }
}

void WeightedSpanTermExtractor::extractWeightedTerms( CL_NS(search)::Query * pQuery, PositionCheckingMap& terms )
{
    TermSet nonWeightedTerms;
    pQuery->extractTerms( &nonWeightedTerms );
    processNonWeightedTerms( terms, nonWeightedTerms, pQuery->getBoost() );
    clearTermSet( nonWeightedTerms );
}

void WeightedSpanTermExtractor::processNonWeightedTerms( PositionCheckingMap& terms, TermSet& nonWeightedTerms, float_t fBoost )
{
    for( TermSet::iterator iter = nonWeightedTerms.begin(); iter != nonWeightedTerms.end(); iter++ )
    {
        Term * pTerm = *iter;
        if( matchesField( pTerm->field() )) 
        {
            WeightedSpanTerm * pWeightedSpanTerm = _CLNEW WeightedSpanTerm( fBoost, pTerm->text() );
            terms.put( pWeightedSpanTerm );
        }
    }
}

void WeightedSpanTermExtractor::clearTermSet( TermSet& termSet )
{
    for( TermSet::iterator itTerms = termSet.begin(); itTerms != termSet.end(); itTerms++ )
    {
        Term * pTerm = *itTerms;
        _CLLDECDELETE( pTerm );
    }
    termSet.clear();
}

IndexReader * WeightedSpanTermExtractor::getFieldReader()
{
    if( ! m_pFieldReader )
    {
        if( m_pReader && m_nDocId != -1 )
        {
            m_pFieldReader = m_pReader;
        }
        else
        {
            RAMDirectory *  pDirectory = _CLNEW RAMDirectory();
     	    IndexWriter     writer( pDirectory, NULL, true );
            Document        doc;

            writer.setMaxFieldLength( LUCENE_INT32_MAX_SHOULDBE );
            
            Field * pField = _CLNEW Field( m_tszFieldName, Field::STORE_NO | Field::INDEX_TOKENIZED );
            pField->setValue( m_pTokenStream );
            doc.add( *pField );

            writer.addDocument( &doc );
            writer.close();

            m_pFieldReader = IndexReader::open( pDirectory, true );
            _CLDECDELETE( pDirectory );
        }
    }

    return m_pFieldReader;
}

void WeightedSpanTermExtractor::closeFieldReader()
{
    if( m_pFieldReader && m_pFieldReader != m_pReader )
    {
        m_pFieldReader->close();
        _CLDELETE( m_pFieldReader );
    }

    m_pFieldReader = NULL;
}

CL_NS_END2
  
