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

CL_CLASS_DEF(analysis,TokenStream);
CL_CLASS_DEF(index,IndexReader);

CL_CLASS_DEF(search,BooleanQuery);
CL_CLASS_DEF(search,PhraseQuery);
CL_CLASS_DEF(search,MultiPhraseQuery);
CL_CLASS_DEF(search,ConstantScoreRangeQuery);
CL_CLASS_DEF2(search,spans,SpanQuery);
CL_CLASS_DEF2(search,highlight,WeightedSpanTerm);

CL_NS_DEF2(search,highlight)

/**
 * Class used to extract {@link WeightedSpanTerm}s from a {@link Query} based on whether Terms from the query are contained in a supplied TokenStream.
 */
class CLUCENE_CONTRIBS_EXPORT WeightedSpanTermExtractor : LUCENE_BASE
{
public:
    /**
     * Map holding Weighted span terms
     */
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

protected:
    const TCHAR *                               m_tszFieldName;
    CL_NS(analysis)::TokenStream *              m_pTokenStream;
    
    CL_NS(index)::IndexReader *                 m_pReader;
    CL_NS(index)::IndexReader *                 m_pFieldReader;
    int32_t                                     m_nDocId;

    bool                                        m_bAutoRewriteQueries;
    bool                                        m_bScoreTerms;

public:
    /**
     * Creates the extractor
     * @param autoRewriteQueries        - should we try to rewrite not rewritten queries and evaluate ConstantScoreRangeQueries
     */
    WeightedSpanTermExtractor( bool autoRewriteQueries = false );
    
    /// Destructor
    virtual ~WeightedSpanTermExtractor();

    /**
     * Sets index reader used to score terms, and if docId is specified in init() then also to rewrite queries and evaluate span queries
     * @param reader                    used to score each term and evaluate the queries
     */
    void setIndexReader( CL_NS(index)::IndexReader * pReader );

    /**
     * Should the index reader be used to score each terms based on its idf?
     */
    void setScoreTerms( bool bScoreTerms );
    bool scoreTerms();

    /**
     * @return whether ConstantScoreRangeQueries are set to be highlighted and other queries rewritten
     */
    void setAutoRewriteQueries( bool bAutoRewriteQueries );
    bool autoRewriteQueries();

    /**
     * Creates a Map of <code>WeightedSpanTerms</code> from the given <code>Query</code> and <code>TokenStream</code>. 
     * 
     * @param weightedSpanTerms         result, this map will be filled with terms
     * @param pQuery                    query that caused hit
     * @param tszFieldName              restricts Term's used based on field name
     * @param pTokenFilter              text to be scored
     * @param nDocId                    id of document to be scored
     * @throws IOException
     */
    void extractWeightedSpanTerms( WeightedSpanTermMap& weightedSpanTerms, CL_NS(search)::Query * pQuery, const TCHAR * tszFieldName, CL_NS(analysis)::TokenStream * pTokenStream, int32_t nDocId = -1 );

protected:
    /**
     * Fills a <code>Map</code> with <@link WeightedSpanTerm>s using the terms from the supplied <code>Query</code>.
     * 
     * @param query         Query to extract Terms from
     * @param terms         Map to place created WeightedSpanTerms in
     * @throws IOException
     */
    void extract( CL_NS(search)::Query * pQuery, PositionCheckingMap& terms );

    /**
     * Methods for extracting info from different types of queries
     */
    void rewriteAndExtract( Query * pQuery, WeightedSpanTermExtractor::PositionCheckingMap& terms );
    void extractFromBooleanQuery( BooleanQuery * pQuery, WeightedSpanTermExtractor::PositionCheckingMap& terms );
    void extractFromPhraseQuery( PhraseQuery * pQuery, WeightedSpanTermExtractor::PositionCheckingMap& terms );
    void extractFromMultiPhraseQuery( MultiPhraseQuery * pQuery, WeightedSpanTermExtractor::PositionCheckingMap& terms );
    void extractFromConstantScoreRangeQuery( ConstantScoreRangeQuery * pQuery, WeightedSpanTermExtractor::PositionCheckingMap& terms );
    
    /**
     * Overwrite this method to handle custom queries
     */
    virtual void extractFromCustomQuery( CL_NS(search)::Query * pQuery, WeightedSpanTermExtractor::PositionCheckingMap& terms );

    /**
     * Fills a <code>Map</code> with <@link WeightedSpanTerm>s using the terms from the supplied <code>SpanQuery</code>.
     * 
     * @param terms         Map to place created WeightedSpanTerms in
     * @param spanQuery     SpanQuery to extract Terms from
     * @throws IOException
     */
    void extractWeightedSpanTerms( CL_NS2(search,spans)::SpanQuery * pSpanQuery, PositionCheckingMap& terms );

    /**
     * Fills a <code>Map</code> with <@link WeightedSpanTerm>s using the terms from the supplied <code>Query</code>.
     * 
     * @param terms         Map to place created WeightedSpanTerms in
     * @param query         Query to extract Terms from
     * @throws IOException
     */
    void extractWeightedTerms( CL_NS(search)::Query * pQuery, PositionCheckingMap& terms );
    void processNonWeightedTerms( PositionCheckingMap& terms, TermSet& nonWeightedTerms, float_t fBoost );

    /**
     * Checks the field to match the current field
     */
    bool matchesField( const TCHAR * tszFieldNameToCheck );

    /**
     * Returns reader for the current field - it returns the supplied reader if the docid is specified, otherwise it creates a new on
     */
    CL_NS(index)::IndexReader * getFieldReader();

    /**
     * Closes allocated reader
     */
    void closeFieldReader();

    /**
     * Clears given term set
     */
    void clearTermSet( TermSet& termSet );
};

CL_NS_END2
#endif // _lucene_search_highlight_weightedspantermextractor_