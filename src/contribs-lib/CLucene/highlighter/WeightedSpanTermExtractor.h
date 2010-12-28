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

#ifndef _lucene_search_highlight_weightedspantermextractor_
#define _lucene_search_highlight_weightedspantermextractor_

#include "CLucene/search/Query.h"
#include "CLucene/highlighter/SpanHighlightScorer.h"

CL_CLASS_DEF(analysis,CachingTokenFilter);
CL_CLASS_DEF(analysis,TokenStream);
CL_CLASS_DEF(index,IndexReader);

CL_CLASS_DEF2(search,spans,SpanQuery);
CL_CLASS_DEF2(search,highlight,WeightedSpanTerm);

CL_NS_DEF2(search,highlight)

/**
 * Class used to extract {@link WeightedSpanTerm}s from a {@link Query} based on whether Terms from the query are contained in a supplied TokenStream.
 */
class CLUCENE_CONTRIBS_EXPORT WeightedSpanTermExtractor : LUCENE_BASE
{
public:
    class PositionCheckingMap
    {
    public:
        WeightedSpanTermMap&    mapSpanTerms;

    public:
        PositionCheckingMap( WeightedSpanTermMap& weightedSpanTerms );
        virtual ~PositionCheckingMap();

        void putAll( PositionCheckingMap * m );
        void put( WeightedSpanTerm * spanTerm );
        WeightedSpanTerm * get( const TCHAR * term );
    };

private:
    const TCHAR *                               fieldName;
    CL_NS(analysis)::CachingTokenFilter *       cachedTokenFilter;
    map<tstring, CL_NS(index)::IndexReader *>   readers;
    TCHAR *                                     defaultField;
    bool                                        highlightCnstScrRngQuery;

public:
    WeightedSpanTermExtractor();
    WeightedSpanTermExtractor( const TCHAR * defaultField );
    
    virtual ~WeightedSpanTermExtractor();

    /**
     * Creates a Map of <code>WeightedSpanTerms</code> from the given <code>Query</code> and <code>TokenStream</code>. Uses a supplied
     * <code>IndexReader</code> to properly weight terms (for gradient highlighting).
     * 
     * <p>
     * 
     * @param query
     *          that caused hit
     * @param tokenStream
     *          of text to be highlighted
     * @param fieldName
     *          restricts Term's used based on field name
     * @param reader
     *          to use for scoring
     * @return
     * @throws IOException
     */
    void getWeightedSpanTermsWithScores( WeightedSpanTermMap& weightedSpanTerms, CL_NS(search)::Query * query, CL_NS(analysis)::CachingTokenFilter * cachingTokenFilter, const TCHAR * fieldName, CL_NS(index)::IndexReader * reader );

    /**
     * Creates a Map of <code>WeightedSpanTerms</code> from the given <code>Query</code> and <code>TokenStream</code>.
     * 
     * <p>
     * 
     * @param query
     *          that caused hit
     * @param tokenStream
     *          of text to be highlighted
     * @return
     * @throws IOException
     */
    void getWeightedSpanTerms( WeightedSpanTermMap& weightedSpanTerms, CL_NS(search)::Query * query, CL_NS(analysis)::CachingTokenFilter * cachingTokenFilter );

    /**
     * Creates a Map of <code>WeightedSpanTerms</code> from the given <code>Query</code> and <code>TokenStream</code>.
     * 
     * <p>
     * 
     * @param query
     *          that caused hit
     * @param tokenStream
     *          of text to be highlighted
     * @param fieldName
     *          restricts Term's used based on field name
     * @return
     * @throws IOException
     */
    void getWeightedSpanTerms( WeightedSpanTermMap& weightedSpanTerms, CL_NS(search)::Query * query, CL_NS(analysis)::CachingTokenFilter * cachingTokenFilter, const TCHAR * fieldName );

    /**
     * Const score range query highlight support
     */
    bool isHighlightCnstScrRngQuery();
    void setHighlightCnstScrRngQuery( bool highlightCnstScrRngQuery );


protected:
    void closeReaders();

    /**
     * Fills a <code>Map</code> with <@link WeightedSpanTerm>s using the terms from the supplied <code>Query</code>.
     * 
     * @param query
     *          Query to extract Terms from
     * @param terms
     *          Map to place created WeightedSpanTerms in
     * @throws IOException
     */
    void extract( CL_NS(search)::Query * query, PositionCheckingMap& terms );

    /**
     * To implement the extraction from custom types of queries overwrite this method
     */
    void extractFromCustomQuery(CL_NS(search)::Query * query, PositionCheckingMap& terms );

    /**
     * Fills a <code>Map</code> with <@link WeightedSpanTerm>s using the terms from the supplied <code>SpanQuery</code>.
     * 
     * @param terms
     *          Map to place created WeightedSpanTerms in
     * @param spanQuery
     *          SpanQuery to extract Terms from
     * @throws IOException
     */
    void extractWeightedSpanTerms( PositionCheckingMap& terms, CL_NS2(search,spans)::SpanQuery * spanQuery );

    /**
     * Fills a <code>Map</code> with <@link WeightedSpanTerm>s using the terms from the supplied <code>Query</code>.
     * 
     * @param terms
     *          Map to place created WeightedSpanTerms in
     * @param query
     *          Query to extract Terms from
     * @throws IOException
     */
    void extractWeightedTerms( PositionCheckingMap& terms, CL_NS(search)::Query * query );
    void processNonWeightedTerms( PositionCheckingMap& terms, TermSet& nonWeightedTerms, float_t fBoost );

    /**
     * Necessary to implement matches for queries against <code>defaultField</code>
     */
    bool fieldNameComparator( const TCHAR * fieldNameToCheck );

    CL_NS(index)::IndexReader * getReaderForField( const TCHAR * field );

    void clearTermSet( TermSet& termSet );
};

CL_NS_END2
#endif // _lucene_search_highlight_weightedspantermextractor_