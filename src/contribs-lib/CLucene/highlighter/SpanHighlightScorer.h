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
 *        - to highlight texts based on unrewriten queries (FuzzyQuery, WildcardQuery, etc)
 *        set autoRewriteQueries=true. Those queries will be rewritten in place according to
 *        data supplied using the tokenStream.
 */
class CLUCENE_CONTRIBS_EXPORT SpanHighlightScorer : public HighlightScorer
{
private:
	CL_NS(util)::CLHashSet<const TCHAR*,
		CL_NS(util)::Compare::TChar,
		CL_NS(util)::Deletor::Dummy>            foundTerms;

    WeightedSpanTermMap                         fieldWeightedSpanTerms;
    
    float_t                                     totalScore;
    float_t                                     maxTermWeight;
    int32_t                                     position;
    bool                                        autoRewriteQueries;         // If this flag is true, than unrewritten queries will be rewritten and constantRangeQueries will be highlighted
    bool                                        deleteWeightedSpanTerms;    // Flag to delete the content of the fieldWeightedSpanTerms

public:
    /**
     * @param query                     Query to use for highlighting
     * @param field                     Field to highlight - pass null to ignore fields
     * @param cachingTokenFilter        of source text to be highlighted
     * @param autoRewriteQuery          try to rewrite a not rewritten queries and highlight ConstantScoreRangeQueries
     * @throws IOException
     */
    SpanHighlightScorer( CL_NS(search)::Query * query, const TCHAR * field, CL_NS(analysis)::TokenStream * tokenStream, bool autoRewriteQueries = false, WeightedSpanTermExtractor * customExtractor = NULL );

    /**
     * @param query                     Query to use for highlighting
     * @param field                     Field to highlight - pass null to ignore fields
     * @param cachingTokenFilter        of source text to be highlighted
     * @param reader                    used to score each term
     * @param autoRewriteQuery          try to rewrite unrewritten queries and highlight ConstantScoreRangeQueries
     * @throws IOException
     */
    SpanHighlightScorer( CL_NS(search)::Query * query, const TCHAR * field, CL_NS(analysis)::TokenStream * tokenStream, CL_NS(index)::IndexReader * reader, bool autoRewriteQueries = false, WeightedSpanTermExtractor * customExtractor = NULL );

    /**
     * @param weightedTerms
     * @param autoRewriteQuery          try to rewrite unrewritten queries and highlight ConstantScoreRangeQueries
     */
    SpanHighlightScorer( WeightedSpanTerm ** weightedTerms, size_t nCount, bool deleteTerms, bool autoRewriteQueries = false );

    /// Destructor
    virtual ~SpanHighlightScorer();

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
     * If you call Highlighter#getBestFragment() more than once you must reset
     * the SpanScorer between each call.
     */
    void reset();

    /**
     * @see org.apache.lucene.search.highlight.Scorer#startFragment(org.apache.lucene.search.highlight.TextFragment)
     */
	virtual void startFragment( TextFragment * newFragment );

    /**
     * @return whether ConstantScoreRangeQueries are set to be highlighted and other queries rewritten
     */
    bool isAutoRewritingQueries();

private:
    /**
     * @param query
     * @param field
     * @param tokenStream
     * @param reader
     * @throws IOException
     */
    void init( CL_NS(search)::Query * query, const TCHAR * field, CL_NS(analysis)::TokenStream * tokenStream, CL_NS(index)::IndexReader * reader, WeightedSpanTermExtractor * customExtractor );
};

CL_NS_END2
#endif // _lucene_search_highlight_spanhighlightscorer_