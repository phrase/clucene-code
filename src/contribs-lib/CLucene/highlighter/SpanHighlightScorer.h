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
CL_CLASS_DEF(analysis, CachingTokenFilter)
CL_CLASS_DEF(search, Query)

CL_NS_DEF2(search,highlight)
class TextFragment;
class WeightedSpanTerm;

/**
 * Adds to the score for a fragment based on its tokens
 */
class CLUCENE_CONTRIBS_EXPORT SpanHighlightScorer : public HighlightScorer
{
private:
    float_t                                     totalScore;
    set<tstring>                                foundTerms;
    map<tstring, WeightedSpanTerm *>            fieldWeightedSpanTerms;
    float_t                                     maxTermWeight;
    int32_t                                     position;
    TCHAR *                                     defaultField;
    bool                                        highlightCnstScrRngQuery;
    bool                                        deleteWeightedSpanTerms;

public:
    SpanHighlightScorer( CL_NS(search)::Query * query, const TCHAR * field, CL_NS(analysis)::CachingTokenFilter * cachingTokenFilter, bool hilitCnstScrRngQuery = false );

    /**
     * @param query
     *            Query to use for highlighting
     * @param field
     *            Field to highlight - pass null to ignore fields
     * @param tokenStream
     *            of source text to be highlighted
     * @param reader
     * @throws IOException
     */
    SpanHighlightScorer( CL_NS(search)::Query * query, const TCHAR * field, CL_NS(analysis)::CachingTokenFilter * cachingTokenFilter, CL_NS(index)::IndexReader * reader, bool hilitCnstScrRngQuery = false );

    /**
     * As above, but with ability to pass in an <tt>IndexReader</tt>
     */
    SpanHighlightScorer( CL_NS(search)::Query * query, const TCHAR * field, CL_NS(analysis)::CachingTokenFilter * cachingTokenFilter, CL_NS(index)::IndexReader * reader, const TCHAR * _defaultField, bool hilitCnstScrRngQuery = false );

    /**
     * @param defaultField - The default field for queries with the field name unspecified
     */
    SpanHighlightScorer( CL_NS(search)::Query * query, const TCHAR * field, CL_NS(analysis)::CachingTokenFilter * cachingTokenFilter, const TCHAR * _defaultField, bool hilitCnstScrRngQuery = false );

    /**
     * @param weightedTerms
     */
    SpanHighlightScorer( WeightedSpanTerm ** weightedTerms, size_t nCount, bool deleteTerms, bool hilitCnstScrRngQuery = false );

    /// Destructor
    virtual ~SpanHighlightScorer();

    /*
     * (non-Javadoc)
     *
     * @see org.apache.lucene.search.highlight.Scorer#getFragmentScore()
     */
    virtual float_t getFragmentScore();

    /**
     *
     * @return The highest weighted term (useful for passing to
     *         GradientFormatter to set top end of coloring scale.
     */
    float_t getMaxTermWeight();

    /*
     * (non-Javadoc)
     *
     * @see org.apache.lucene.search.highlight.Scorer#getTokenScore(org.apache.lucene.analysis.Token,
     *      int)
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

    /*
     * (non-Javadoc)
     *
     * @see org.apache.lucene.search.highlight.Scorer#startFragment(org.apache.lucene.search.highlight.TextFragment)
     */
	virtual void startFragment( TextFragment * newFragment );

    /**
     * @return whether ConstantScoreRangeQuerys are set to be highlighted
     */
    bool isHighlightCnstScrRngQuery();

    /**
     * Turns highlighting of ConstantScoreRangeQuery on/off. ConstantScoreRangeQuerys cannot be
     * highlighted if you rewrite the query first. Must be called before SpanScorer construction.
     * 
     * @param highlightCnstScrRngQuery
     */
    void setHighlightCnstScrRngQuery( bool highlight );

private:
    /**
     * @param query
     * @param field
     * @param tokenStream
     * @param reader
     * @throws IOException
     */
    void init( CL_NS(search)::Query * query, const TCHAR * field, CL_NS(analysis)::CachingTokenFilter * cachingTokenFilter, CL_NS(index)::IndexReader * reader );
};

CL_NS_END2
#endif // _lucene_search_highlight_spanhighlightscorer_