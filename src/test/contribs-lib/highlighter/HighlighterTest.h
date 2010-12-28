/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "test.h"

#include "CLucene/analysis/Analyzers.h"
#include "CLucene/index/IndexReader.h"
#include "CLucene/index/IndexWriter.h"
#include "CLucene/search/Hits.h"
#include "CLucene/search/Query.h"
#include "CLucene/search/SearchHeader.h"
#include "CLucene/search/spans/SpanTermQuery.h"
#include "CLucene/highlighter/TokenGroup.h"
#include "CLucene/store/RAMDirectory.h"
#include "CLucene/highlighter/Formatter.h"

CL_NS_USE(analysis);
CL_NS_USE(index);
CL_NS_USE(store);
CL_NS_USE(search);
CL_NS_USE2(search,highlight);
CL_NS_USE2(search,spans);

#define FIELD_NAME _T( "contents" )


/////////////////////////////////////////////////////////////////////////////
class HelperFormatter : public Formatter
{
public:
    int32_t                 numHighlights;

public:
    HelperFormatter ();
	virtual ~HelperFormatter();
    virtual TCHAR* highlightTerm( const TCHAR* originalTermText, const TokenGroup* tokenGroup );
    void reset();
};


/////////////////////////////////////////////////////////////////////////////
class HighlighterTest 
{
protected:
    CuTest*                 tc;
    IndexReader *           reader;
    RAMDirectory *          ramDir;
    Query *                 query;
    Searcher *              searcher;
    Hits *                  hits;
    StandardAnalyzer        analyzer;
    Directory *             dir;
    static const TCHAR*     texts[];
    HelperFormatter         formatter;

public:
    HighlighterTest( CuTest* tc );
    virtual ~HighlighterTest();

    void testSimpleSpanHighlighter();
    void testSimpleSpanPhraseHighlighting();
    void testSimpleSpanPhraseHighlighting2();
    void testSimpleSpanPhraseHighlighting3();
    void testPosTermStdTerm();
    void testSpanMultiPhraseQueryHighlighting();
    void testSpanMultiPhraseQueryHighlightingWithGap();
    void testNearSpanSimpleQuery();
    void testSpanHighlighting();
    void testNotSpanSimpleQuery();
    void testGetBestFragmentsSimpleQuery();
    void testGetFuzzyFragments();
    void testGetWildCardFragments();
    void testGetMidWildCardFragments();
    void testGetRangeFragments();
    void testGetConstantScoreRangeFragments();
    void testGetBestFragmentsPhrase();
    void testGetBestFragmentsSpan();
//     void testGetBestFragmentsFilteredQuery();
//     void testGetBestFragmentsFilteredPhraseQuery();
    void testGetBestFragmentsMultiTerm();
    void testGetBestFragmentsWithOr();
    void testGetBestSingleFragment();
    void testGetBestSingleFragmentWithWeights();
    void testUnRewrittenQuery();
    void testNoFragments();

//     NOT IMPLEMENTED
//     void testOffByOne();
//     void testOverlapAnalyzer();
//     void testGetSimpleHighlight();
//     void testGetTextFragments();
//     void testMaxSizeHighlight();
//     void testMaxSizeHighlightTruncates();  
//     void testMaxSizeEndHighlight();
//     void testEncoding();
//     void testMultiSearcher();
//     void testFieldSpecificHighlighting();
//     void testOverlapAnalyzer2(); 
//     void testWeightedTermsWithDeletes();

    const TCHAR * highlightTerm( const TCHAR * originalText, TokenGroup * group );
    void doSearching( const TCHAR * queryString );
    void doSearching( Query * unReWrittenQuery, bool bRememberRewritten = true );


protected:
    TCHAR * highlightField( Query * query, const TCHAR * fieldName, const TCHAR * text );
    void assertHighlightAllHits( int32_t maxFragments, int32_t fragmentSize, int32_t expectedHighlights, bool autoRewriteQuery = false );
    Token * createToken( const TCHAR * term, int32_t start, int32_t offset );

    SpanTermQuery * createSpanTermQuery( const TCHAR * fieldName, const TCHAR * text );

    TokenStream * getTS2();
    TokenStream * getTS2a();

    Document * doc( const TCHAR * f, const TCHAR * v );
    void makeIndex();
    void deleteDocument();
    void searchIndex();

    void setUp();
    void tearDown();
};

