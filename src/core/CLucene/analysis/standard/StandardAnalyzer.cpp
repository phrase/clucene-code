/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "StandardAnalyzer.h"

////#include "CLucene/util/VoidMap.h"
#include "CLucene/util/CLStreams.h"
#include "CLucene/analysis/AnalysisHeader.h"
#include "CLucene/analysis/Analyzers.h"
#include "StandardFilter.h"
#include "StandardTokenizer.h"

CL_NS_USE(util)
CL_NS_USE(analysis)

CL_NS_DEF2(analysis,standard)

  class StandardAnalyzer::SavedStreams : public TokenStream {
  public:
    StandardTokenizer* tokenStream;
    TokenStream* filteredTokenStream;

    SavedStreams(){
      tokenStream = NULL;
      filteredTokenStream = NULL;
    }

    Token* next(Token* token){
      return token;
    }
    void close(){}
    void reset(){}
    virtual ~SavedStreams(){
    
    }
  };

	StandardAnalyzer::StandardAnalyzer():
		stopSet(_CLNEW CLTCSetList(true))
	{
      StopFilter::fillStopTable( stopSet,CL_NS(analysis)::StopAnalyzer::ENGLISH_STOP_WORDS);
	}

	StandardAnalyzer::StandardAnalyzer( const TCHAR** stopWords):
		stopSet(_CLNEW CLTCSetList(true))
	{
		StopFilter::fillStopTable( stopSet,stopWords );
	}

	StandardAnalyzer::StandardAnalyzer(const char* stopwordsFile, const char* enc):
		stopSet(_CLNEW CLTCSetList(true))
	{
		if ( enc == NULL )
			enc = "ASCII";
		WordlistLoader::getWordSet(stopwordsFile, enc, stopSet);
	}

	StandardAnalyzer::StandardAnalyzer(CL_NS(util)::Reader* stopwordsReader, const bool _bDeleteReader):
		stopSet(_CLNEW CLTCSetList(true))
	{
		WordlistLoader::getWordSet(stopwordsReader, stopSet, _bDeleteReader);
	}

	StandardAnalyzer::~StandardAnalyzer(){
		_CLDELETE(stopSet);
	}
	TokenStream* StandardAnalyzer::tokenStream(const TCHAR* fieldName, Reader* reader)
	{
    BufferedReader* bufferedReader = reader->__asBufferedReader();

    StandardTokenizer* tokenStream;
    if ( bufferedReader == NULL )
      tokenStream =  _CLNEW StandardTokenizer( _CLNEW FilteredBufferedReader(reader, false), true );
    else
      tokenStream = _CLNEW StandardTokenizer(bufferedReader);

    TokenStream* result = _CLNEW StandardFilter(tokenStream,true);
    result = _CLNEW LowerCaseFilter(result,true);
    result = _CLNEW StopFilter(result,true, stopSet);
    return result;
  }
  
	TokenStream* StandardAnalyzer::reusableTokenStream(const TCHAR* fieldName, Reader* reader)
	{
    SavedStreams* streams = (SavedStreams*)getPreviousTokenStream();
    if (streams == NULL) {
      streams = _CLNEW SavedStreams;
      setPreviousTokenStream(streams);

      BufferedReader* bufferedReader = reader->__asBufferedReader();

      if ( bufferedReader == NULL )
        streams->tokenStream =  _CLNEW StandardTokenizer( _CLNEW FilteredBufferedReader(reader, false), true );
      else
        streams->tokenStream = _CLNEW StandardTokenizer(bufferedReader);

      streams->filteredTokenStream = _CLNEW StandardFilter(streams->tokenStream,true);
      streams->filteredTokenStream = _CLNEW LowerCaseFilter(streams->filteredTokenStream,true);
      streams->filteredTokenStream = _CLNEW StopFilter(streams->filteredTokenStream,true, stopSet);
      
    } else{
      streams->tokenStream->reset(reader);
    }
    //ret->setMaxTokenLength(maxTokenLength);
    //ret->setReplaceInvalidAcronym(replaceInvalidAcronym);
    return streams->filteredTokenStream;
	}
CL_NS_END2
