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
#include "CLucene/analysis/CachingTokenFilter.h"

#include "CLucene/search/Query.h"
#include "CLucene/search/BooleanQuery.h"
#include "CLucene/search/BooleanClause.h"
#include "CLucene/search/PhraseQuery.h"
#include "CLucene/search/TermQuery.h"
#include "CLucene/search/MultiPhraseQuery.h"
#include "CLucene/search/ConstantScoreQuery.h"

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

WeightedSpanTermExtractor::WeightedSpanTermExtractor()
{
    fieldName = NULL;
    cachedTokenFilter = NULL;
    defaultField = NULL;
    highlightCnstScrRngQuery = false;
}

WeightedSpanTermExtractor::WeightedSpanTermExtractor( const TCHAR * _defaultField )
{
    fieldName = NULL;
    cachedTokenFilter = NULL;
    defaultField = NULL;
    highlightCnstScrRngQuery = false;

    if( _defaultField )
        defaultField = STRDUP_TtoT( _defaultField );
}

WeightedSpanTermExtractor::~WeightedSpanTermExtractor()
{
    _CLDELETE_ARRAY( defaultField );
}

void WeightedSpanTermExtractor::closeReaders()
{
    for( map<tstring, IndexReader *>::iterator iReader = readers.begin(); iReader != readers.end(); iReader++ )
    {
        iReader->second->close();
        _CLDELETE( iReader->second );
    }

    readers.clear();
}

void WeightedSpanTermExtractor::getWeightedSpanTermsWithScores( WeightedSpanTermMap& weightedSpanTerms, Query * query, CachingTokenFilter * cachingTokenFilter, const TCHAR * _fieldName, IndexReader * reader )
{
    fieldName = _fieldName;
    cachedTokenFilter = cachingTokenFilter;

    WeightedSpanTermExtractor::PositionCheckingMap terms( weightedSpanTerms );
    extract( query, terms );

    int32_t totalNumDocs = reader->numDocs();
    Term * term = _CLNEW Term();
    
    for( WeightedSpanTermMap::iterator iSpanTerm = terms.mapSpanTerms.begin(); iSpanTerm != terms.mapSpanTerms.end(); iSpanTerm++ )
    {
        term->set( fieldName, iSpanTerm->first );
        int32_t docFreq = reader->docFreq( term );

        if( totalNumDocs < docFreq )
            docFreq = totalNumDocs;

        // IDF algorithm taken from DefaultSimilarity class
        float_t idf = (float_t)( log( (float_t)totalNumDocs / (float_t)(docFreq + 1) ) + 1.0f );
        iSpanTerm->second->setWeight( iSpanTerm->second->getWeight() * idf );
    }

    closeReaders();
    cachedTokenFilter->close();
    _CLDELETE( cachedTokenFilter );
    fieldName = NULL;
}

void WeightedSpanTermExtractor::getWeightedSpanTerms( WeightedSpanTermMap& weightedSpanTerms, Query * query, CachingTokenFilter * cachingTokenFilter )
{
    getWeightedSpanTerms( weightedSpanTerms, query, cachingTokenFilter, NULL );
}

void WeightedSpanTermExtractor::getWeightedSpanTerms( WeightedSpanTermMap& weightedSpanTerms, Query * query, CachingTokenFilter * cachingTokenFilter, const TCHAR * _fieldName )
{
    fieldName = _fieldName;
    cachedTokenFilter = cachingTokenFilter;

    WeightedSpanTermExtractor::PositionCheckingMap terms( weightedSpanTerms );
    extract( query, terms );
    closeReaders();
    
    cachedTokenFilter = NULL;
    fieldName = NULL;
}

bool WeightedSpanTermExtractor::isHighlightCnstScrRngQuery()
{
    return highlightCnstScrRngQuery;
}

void WeightedSpanTermExtractor::setHighlightCnstScrRngQuery( bool _highlightCnstScrRngQuery )
{
    highlightCnstScrRngQuery = _highlightCnstScrRngQuery;
}

bool WeightedSpanTermExtractor::fieldNameComparator( const TCHAR * fieldNameToCheck )
{
    return ( fieldName == NULL 
            || fieldNameToCheck == fieldName 
            || 0 == _tcscmp( fieldNameToCheck, fieldName )
            || 0 == _tcscmp( fieldNameToCheck, defaultField ));
}

IndexReader * WeightedSpanTermExtractor::getReaderForField( const TCHAR * field )
{
    map<tstring,IndexReader *>::iterator iReader = readers.find( field );
    if( iReader == readers.end() )
    {
        RAMDirectory * directory = _CLNEW RAMDirectory();
     	IndexWriter writer( directory, NULL, true );
        Document doc;
        
        Field * f = _CLNEW Field( field, Field::STORE_NO | Field::INDEX_TOKENIZED );
        f->setValue( cachedTokenFilter );
        doc.add( *f );

        writer.addDocument( &doc );
        writer.close();

        IndexReader * reader = IndexReader::open( directory, true );
        readers[ field ] = reader;

        _CLDECDELETE( directory );
        return reader;
    }
    else
    {
        return iReader->second;
    }
}

void WeightedSpanTermExtractor::extract( Query * query, WeightedSpanTermExtractor::PositionCheckingMap& terms )
{
    if( query->instanceOf( BooleanQuery::getClassName() ))
    {
        int32_t clauses = ((BooleanQuery*) query)->getClauseCount();
        if( clauses > 0 )
        {
            BooleanClause ** queryClauses = _CL_NEWARRAY( BooleanClause*, clauses );
            ((BooleanQuery*) query)->getClauses( queryClauses );

            for( int32_t i = 0; i < clauses; i++ )
            {
                if( ! queryClauses[ i ]->isProhibited() )
                    extract( queryClauses[ i ]->getQuery(), terms );
            }
            _CLDELETE_ARRAY( queryClauses );
        }
    } 
    else if( query->instanceOf( PhraseQuery::getClassName() )) 
    {
        Term** phraseQueryTerms = ((PhraseQuery *)query)->getTerms();
        vector<SpanQuery *> vSpans;
        for( int32_t i = 0; phraseQueryTerms[ i ]; i++ )
            vSpans.push_back( _CLNEW SpanTermQuery( phraseQueryTerms[ i ] ));

        int32_t slop = ((PhraseQuery *)query)->getSlop();
        bool inorder = ( slop == 0 );

        SpanNearQuery * sp = _CLNEW SpanNearQuery( vSpans.begin(), vSpans.end(), slop, inorder, true );
        sp->setBoost( query->getBoost() );
        extractWeightedSpanTerms(terms, sp);
        _CLDELETE( sp );
        _CLDELETE_ARRAY( phraseQueryTerms );
    } 
    else if( query->instanceOf( TermQuery::getClassName() ))
    {
        extractWeightedTerms( terms, query );
    } 
    else if( query->instanceOf( SpanFirstQuery::getClassName() )
          || query->instanceOf( SpanNearQuery::getClassName() )
          || query->instanceOf( SpanNotQuery::getClassName() )
          || query->instanceOf( SpanOrQuery::getClassName() )
          || query->instanceOf( SpanTermQuery::getClassName() ))
    {
      extractWeightedSpanTerms(terms, (SpanQuery* )query);
    } 
    else if( query->instanceOf( MultiPhraseQuery::getClassName() )) 
    {
        MultiPhraseQuery * mpq = (MultiPhraseQuery *)query;

        const CLArrayList<ArrayBase<Term*>*>* termArrays   = NULL;
        ValueArray<int32_t>                   positions;

        termArrays = mpq->getTermArrays();
        mpq->getPositions( positions );

        if( positions.length > 0 )
        {
            int32_t maxPosition = positions[ positions.length - 1 ];
            for( size_t i = 0; i < positions.length - 1; i++ )
            {
                if( positions[ i ] > maxPosition )
                    maxPosition = positions[ i ];
            }

            vector<SpanTermQuery *>** disjunctLists = _CL_NEWARRAY( vector<SpanTermQuery *>*, maxPosition + 1 );
            memset( disjunctLists, 0, sizeof( vector<SpanTermQuery *>* ) * ( maxPosition + 1 ) );
            int32_t distinctPositions = 0;
            for( size_t i = 0; i < termArrays->size(); i++ )
            {
                ArrayBase<Term*> * termArray = termArrays->at( i );
                if( ! disjunctLists[ positions[ i ]] )
                    disjunctLists[ positions[ i ]] = _CLNEW vector<SpanTermQuery *>;
                vector<SpanTermQuery *>& disjuncts = *( disjunctLists[ positions[ i ]] );
                if( disjuncts.empty() )
                    distinctPositions++;
        
                for( size_t j = 0; j < termArray->length; j++ )
                    disjuncts.push_back( _CLNEW SpanTermQuery( (*termArray)[ j ] ));
            }

            int32_t positionGaps = 0;
            int32_t position = 0;
            SpanQuery** clauses = _CL_NEWARRAY( SpanQuery*, distinctPositions );
            for( int32_t i = 0; i <= maxPosition; i++ )
            {
                vector<SpanTermQuery *> * pDisjuncts = disjunctLists[ i ];
                if( pDisjuncts && ! pDisjuncts->empty() )
                    clauses[ position++ ] = _CLNEW SpanOrQuery( pDisjuncts->begin(), pDisjuncts->end(), true );
                else
                    positionGaps++;
            }

            int32_t slop = mpq->getSlop();
            bool inorder = (slop == 0);

            SpanNearQuery * sp = _CLNEW SpanNearQuery( clauses, clauses+distinctPositions, slop + positionGaps, inorder, true );
            sp->setBoost( query->getBoost() );
            extractWeightedSpanTerms( terms, sp );

            _CLDELETE( sp );
            _CLDELETE_ARRAY( clauses );
            for( int32_t i = 0; i <= maxPosition; i++ )
                _CLDELETE( disjunctLists[ i ] );
            _CLDELETE_ARRAY( disjunctLists ); 
        }
    } 
    else if( highlightCnstScrRngQuery && query->instanceOf( ConstantScoreRangeQuery::getClassName() ))
    {
        ConstantScoreRangeQuery * q = (ConstantScoreRangeQuery *) query;
        if( fieldNameComparator( q->getField() ))
        {
            Term * lower = _CLNEW Term( q->getField(), q->getLowerVal() );
            Term * upper = _CLNEW Term( q->getField(), q->getUpperVal() );

            IndexReader * reader = getReaderForField( fieldName );
            try 
            {
                TermEnum * te = reader->terms( lower );
                TermSet setTerms;
                do 
                {
                    Term * term = te->term( false );
                    if( term && upper->compareTo( term ) >= 0 )
                        setTerms.insert( term );
                    else 
                        break;
                }
                while( te->next() );

                processNonWeightedTerms( terms, setTerms, query->getBoost() );
                _CLLDELETE( te );
            }
            catch( ... )
            {
                reader->close();
                _CLLDECDELETE( upper );
                _CLLDECDELETE( lower );
                throw;
            }
            reader->close();
            _CLLDECDELETE( upper );
            _CLLDECDELETE( lower );
        }
    }
    else
    {
        extractFromCustomQuery( query, terms );
    }
}

void WeightedSpanTermExtractor::extractFromCustomQuery(CL_NS(search)::Query * query, PositionCheckingMap& terms )
{
}

void WeightedSpanTermExtractor::extractWeightedSpanTerms( PositionCheckingMap& terms, CL_NS2(search,spans)::SpanQuery * spanQuery )
{
    set<tstring>    fieldNames;
    TermSet         nonWeightedTerms;

    spanQuery->extractTerms( &nonWeightedTerms );
    if( ! fieldName ) 
    {
        for( TermSet::iterator iter = nonWeightedTerms.begin(); iter != nonWeightedTerms.end(); iter++ )
        {
            tstring tsFieldName = (*iter)->field();
            if( fieldNames.end() == fieldNames.find( tsFieldName ))
                fieldNames.insert( tsFieldName );
        }
    }
    else 
        fieldNames.insert( fieldName );

    // To support the use of the default field name
    if( defaultField )
        fieldNames.insert( defaultField );

    vector<WeightedSpanTerm::PositionSpan *> spanPositions;
    for( set<tstring>::iterator iFieldName = fieldNames.begin(); iFieldName != fieldNames.end(); iFieldName++ )
    {
        IndexReader * reader = getReaderForField( iFieldName->c_str() );
        Spans * spans = spanQuery->getSpans( reader );

        // collect span positions
        while( spans->next() )
            spanPositions.push_back( _CLNEW WeightedSpanTerm::PositionSpan( spans->start(), spans->end() - 1 ));

        cachedTokenFilter->reset();
       _CLDELETE( spans );
    }

    if( spanPositions.size() != 0 ) 
    {
        for( TermSet::iterator iter = nonWeightedTerms.begin(); iter != nonWeightedTerms.end(); iter++ )
        {
            Term * queryTerm = *iter;
            if( fieldNameComparator( queryTerm->field() )) 
            {
                WeightedSpanTerm * weightedSpanTerm = terms.get( queryTerm->text() );
                if( ! weightedSpanTerm ) 
                {
                    weightedSpanTerm = _CLNEW WeightedSpanTerm( spanQuery->getBoost(), queryTerm->text() );
                    weightedSpanTerm->addPositionSpans( spanPositions );
                    weightedSpanTerm->setPositionSensitive( true );
                    terms.put( weightedSpanTerm );
                } 
                else 
                {
                    if( spanPositions.size() > 0 )
                        weightedSpanTerm->addPositionSpans( spanPositions );
                }
            }
        }

        for( vector<WeightedSpanTerm::PositionSpan *>::iterator iPS = spanPositions.begin(); iPS != spanPositions.end(); iPS++ )
            _CLLDECDELETE( *iPS );
    }

    clearTermSet( nonWeightedTerms );
}

void WeightedSpanTermExtractor::extractWeightedTerms( PositionCheckingMap& terms, CL_NS(search)::Query * query )
{
    TermSet nonWeightedTerms;
    query->extractTerms( &nonWeightedTerms );
    processNonWeightedTerms( terms, nonWeightedTerms, query->getBoost() );
    clearTermSet( nonWeightedTerms );
}

void WeightedSpanTermExtractor::processNonWeightedTerms( PositionCheckingMap& terms, TermSet& nonWeightedTerms, float_t fBoost )
{
    for( TermSet::iterator iter = nonWeightedTerms.begin(); iter != nonWeightedTerms.end(); iter++ )
    {
        Term * queryTerm = *iter;
        if( fieldNameComparator( queryTerm->field() )) 
        {
            WeightedSpanTerm * weightedSpanTerm = _CLNEW WeightedSpanTerm( fBoost, queryTerm->text() );
            terms.put( weightedSpanTerm );
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

CL_NS_END2
  
