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

#ifndef _lucene_search_highlight_spanhighlightscorer_
#define _lucene_search_highlight_spanhighlightscorer_

#include "HighlightScorer.h"

CL_CLASS_DEF(index, Term)
CL_CLASS_DEF(index, IndexReader)
CL_CLASS_DEF(analysis, Token)
CL_CLASS_DEF(analysis, TokenStream)
CL_CLASS_DEF(search, Query)

CL_NS_DEF2(search,highlight)
class TextFragment;
class WeightedSpanTerm;
class WeightedSpanTermExtractor;


/////////////////////////////////////////////////////////////////////////////
typedef CL_NS(util)::CLHashMap<const TCHAR*, WeightedSpanTerm *,
		CL_NS(util)::Compare::TChar,
		CL_NS(util)::Equals::TChar,
		CL_NS(util)::Deletor::Dummy,
		CL_NS(util)::Deletor::Dummy>
    WeightedSpanTermMap;


/**
 * Adds to the score for a fragment based on its tokens
 * 
 * Notes: - to be able to highlight based on the ConstantScoreRangeQuery you must supply this 
 *        query not rewritten (it rewrites to ConstantScoreQuery which in turn provides no
 *        terms) and use the autoRewriteQuery feature.
 *        - to highlight texts based on not rewritten queries (FuzzyQuery, WildcardQuery, etc)
 *        set autoRewriteQueries=true. Those queries will be rewritten in place according to
 *        data supplied using the tokenStream.
 */
class CLUCENE_CONTRIBS_EXPORT SpanHighlightScorer : public HighlightScorer
{
protected:
	CL_NS(util)::CLHashSet<const TCHAR*,
		CL_NS(util)::Compare::TChar,
		CL_NS(util)::Deletor::Dummy>            m_foundTerms;                   // Extracted terms

    WeightedSpanTermMap                         m_fieldWeightedSpanTerms;       // Extracted span terms
    bool                                        m_bDeleteWeightedSpanTerms;     // Flag to delete the content of the fieldWeightedSpanTerms
    
    float_t                                     m_fTotalScore;                  // The sum of scores of all extracted span terms
    float_t                                     m_fMaxTermWeight;               // Maximal span term score
    int32_t                                     m_nPosition;                    // Current position

    bool                                        m_bAutoRewriteQueries;          // If this flag is true, than not rewritten queries will be rewritten and constantRangeQueries will be highlighted
    bool                                        m_bScoreTerms;                  // Score each term based on the given reader?

    CL_NS(index)::IndexReader *                 m_pReader;                      // Index reader - can be used to evaluate span queries, rewrite not rewritten queries and score terms
    int32_t                                     m_nDocId;                       // Document id withing the index associated with the m_pReader. If specified (!= -1) then the reader is used to eval queries
    
    WeightedSpanTermExtractor *                 m_pSpanExtractor;               // Custom span term extractor can be specified.
    bool                                        m_bDeleteExtractor;             // delete flag for the extractor

    std::vector<int32_t>                             m_vPositions;                   // Vectors of positions that have score greater than 0, sorted
    int32_t                                     m_nNextPositionIdx;             // Next position index withing the m_vPositions vector
    int32_t                                     m_nNextPosition;                // Next position with score > 0
    std::map<int32_t, float_t>                       m_mapPosition2Score;            // Maps position to score
    bool                                        m_bPositionBased;               // Use positions to score tokens
    bool                                        m_bScorePositions;              // Compute score for each position


public:
    /**
     * @param autoRewriteQuery          try to rewrite a not rewritten queries and highlight ConstantScoreRangeQueries
     * @param customExtractor           use this extractor to extract span terms
     * @param bDeleteExtractor          delete the custom extractor flag
     */
    SpanHighlightScorer( bool bAutoRewriteQueries = false, WeightedSpanTermExtractor * pCustomExtractor = NULL, bool bDeleteExtractor = false );

    /**
     * @param weightedTerms             already extracted weighted terms
     * @param nCount                    count of terms in weightedTerms
     * @param deleteTerms               should all the given terms be deleted on shutdown
     */
    SpanHighlightScorer( WeightedSpanTerm ** rgWeightedTerms, size_t nCount, bool bDeleteTerms );

    /// Destructor
    virtual ~SpanHighlightScorer();

    /**
     * Sets index reader used to score terms, and if docId is specified in init() then also to rewrite queries and evaluate span queries
     * @param reader                    used to score each term and evaluate the queries
     */
    void setIndexReader( CL_NS(index)::IndexReader * pReader );
    CL_NS(index)::IndexReader * getIndexReader();

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
     * Defines the way of computing score for each token.
     * @param bPositionBased            if set to true then only positions data is used to compute score of a token, otherwise token text will be used to.
     * @param bScorePositions           if set to true then score for each position is computed separately, if false each position has constant score 1.0f
     */
    void setPositionBased( bool bPositionBased, bool bScorePositions );
    bool isPositionBased();
    bool scoredPositions();

    /**
     * @param query         - initialize the span term scores based on this query
     * @param field         - score terms of this field
     * @param tokenStream   - this token stream defines the document content 
     */
    void init( CL_NS(search)::Query * pQuery, const TCHAR * tszField, CL_NS(analysis)::TokenStream * pTokenStream, int32_t nDocId = -1 );

    /**
     * If you call Highlighter#getBestFragment() more than once you must reset
     * the SpanScorer between each call.
     */
    void reset();

    /**
     * @see org.apache.lucene.search.highlight.Scorer#getFragmentScore()
     */
    virtual float_t getFragmentScore();

    /**
     * @return The highest weighted term (useful for passing to
     *         GradientFormatter to set top end of coloring scale.
     */
    float_t getMaxTermWeight();

    /**
     * @see org.apache.lucene.search.highlight.Scorer#getTokenScore(org.apache.lucene.analysis.Token,int)
     */
    virtual float_t getTokenScore( CL_NS(analysis)::Token * pToken );

    /**
     * Retrieve the WeightedSpanTerm for the specified token. Useful for passing
     * Span information to a Fragmenter.
     *
     * @param token
     * @return WeightedSpanTerm for token
     */
    WeightedSpanTerm * getWeightedSpanTerm( const TCHAR * tszToken );

    /**
     * @see org.apache.lucene.search.highlight.Scorer#startFragment(org.apache.lucene.search.highlight.TextFragment)
     */
	virtual void startFragment( TextFragment * pNewFragment );

protected:

    /**
     * Extracts positions in order to use term positions to score tokens instead of their texts
     */
    void extractTermPositions( const TCHAR * tszField, int32_t nDocId );

    /**
     * Helper method for freeing weighted span terms and their positions
     */
    void freeWeightedSpanTerms();

};

CL_NS_END2
#endif // _lucene_search_highlight_spanhighlightscorer_