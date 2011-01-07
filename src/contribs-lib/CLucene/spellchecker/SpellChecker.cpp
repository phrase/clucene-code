#include "CLucene/_ApiHeader.h"
#include "SpellChecker.h"
#include "SuggestWord.h"
#include "LevenshteinDistance.h"
#include <queue>
#include <vector>
#include <sstream>
#include <string.h>

SpellCheckerC::SpellCheckerC(CL_NS(store)::Directory *dir, StringDistanceC *sd, bool closeDir)
:_fword(_T("word")), _bStart(2.0F), _bGram(1.0F), _bEnd(1.0F), _minScore(0.5F), _stringDistance(NULL), _dir(NULL), _indexSearcher(NULL), _closeDir(closeDir)
{
  if (dir != NULL)
  {
    setSpellIndex(dir);
  }

  if (sd != NULL)
  {
    setStringDistance(sd);
  }
  else
  {
    setStringDistance(new LevenshteinDistanceC());
  }
}


SpellCheckerC::~SpellCheckerC()
{
  if (this->_indexSearcher)
  {
    this->_indexSearcher->close();
    _CLDELETE(this->_indexSearcher)
  }

  if (this->_stringDistance)
  {
    _CLDELETE(this->_stringDistance)
  }

  if (this->_closeDir)
  {
    if (this->_dir)
    {
      this->_dir->close();
      _CLDECDELETE(this->_dir)
    }
  }
}


void SpellCheckerC::setSpellIndex(CL_NS(store)::Directory *dir)
{
  //SCOPED_LOCK_MUTEX(_thisLock)
  if(!CL_NS(index)::IndexReader::indexExists(dir))
  {
    CL_NS(index)::IndexWriter writer(dir, 0, true);
    writer.close();
  }
  this->_dir = dir;
  swapSearcher(this->_dir);
}


void SpellCheckerC::setStringDistance(StringDistanceC *sd)
{
  //SCOPED_LOCK_MUTEX(_thisLock);
  if (this->_stringDistance)
  {
    _CLDELETE(this->_stringDistance)
  }
  this->_stringDistance = sd;
}


void SpellCheckerC::setAccuracy(float minScore)
{
  //SCOPED_LOCK_MUTEX(_thisLock);
  this->_minScore = minScore;
}


bool SpellCheckerC::createFromDictionary(DictionaryC &dict, bool create)
{
  //SCOPED_LOCK_MUTEX(_writeLock);

  // Create IndexWriter for the target spell index.
  CL_NS(document)::Document doc;
  CL_NS(analysis)::WhitespaceAnalyzer analyzer;
  CL_NS(index)::IndexWriter iw(this->_dir, &analyzer, create);

  // Iterate the DictionaryC words.
  while (dict.next())
  {
    const TCHAR *word = dict.word();
    int wordLen = _tcslen(word);

    if (wordLen < 3)
      continue; // too short

    if (exists(word))
      continue; // the word already exists in the index.

    doc.clear();
    this->fillDocument(&doc, word);
    iw.addDocument(&doc);
  }

  // Optimize the index.
  // This could take some time...
  iw.optimize();
  iw.close();
  swapSearcher(_dir);
  return true;
}


const TCHAR** SpellCheckerC::suggestSimilar(const TCHAR *word, int numSug)
{
  if (!word)
    return 0;

  size_t wordLen = _tcslen(word);
  if (wordLen < 3)
    return 0; // The word is too short.

  // Create a search Query object, which finds similar words.
  CL_NS(search)::BooleanQuery query;
  CL_NS(search)::TermQuery *termQuery = 0;
  CL_NS(index)::Term *term = 0;

  int nMin = this->getMinNGramLen(wordLen);
  int nMax = this->getMaxNGramLen(wordLen);
  for (int n=nMin; n<=nMax; n++)
  {
    std::wstringstream ss;
    ss << n;
    std::wstring wsnum = ss.str();
    
    // Fields.    
    std::wstring wsfstart(_T("start"));
    wsfstart += wsnum;

    std::wstring wsfgram(_T("gram"));
    wsfgram += wsnum;

    std::wstring wsfend(_T("end"));
    wsfend += wsnum;

    // Create ngram parts of the search word.
    int grammsLen;
    TCHAR **gramms = this->createNGram(word, n, grammsLen);

    // The "startX" term.
    term = new CL_NS(index)::Term(wsfstart.c_str(), gramms[0]);
    termQuery = new CL_NS(search)::TermQuery(term);
    termQuery->setBoost(this->_bStart);
    query.add(termQuery, false, false);

    // The "gramX" terms.
    for (int i=0; i<grammsLen; i++)
    {
      term = new CL_NS(index)::Term(wsfgram.c_str(), gramms[i]);
      termQuery = new CL_NS(search)::TermQuery(term);
      termQuery->setBoost( this->_bGram );
      query.add(termQuery, false, false);
    }

    // The "endX" term.
    term = new CL_NS(index)::Term( wsfend.c_str(), gramms[grammsLen-1] );
    termQuery = new CL_NS(search)::TermQuery( term );
    termQuery->setBoost( this->_bEnd );
    query.add( termQuery, false, false );

    // Delete gramms.
    _CLDELETE_ARRAY_ALL(gramms);
  }

  // Start the search for the word.
  CL_NS(search)::IndexSearcher *searcher = obtainSearcher();
  CL_NS(search)::Hits *hits = searcher->search( &query );
  size_t hitsCount = hits->length();

  if (hits && hitsCount > 0)
  {
    // Queue, which always holds the best results.
    // Due to the operator overrides from SuggestWordC the lowest scored word will be at the top.
    std::priority_queue<SuggestWordC*, std::vector<SuggestWordC*>, SuggestWordC > queue;
    float min = this->_minScore;
    size_t maxHits = numSug * 10;
    int stop = minimum(maxHits, hitsCount);

    for (size_t i=0; i<stop; i++)
    {
      CL_NS(document)::Document &doc = hits->doc(i);
      const TCHAR *doc_word = doc.get(this->_fword);

      // Do not suggest the same word as the one which is used for search... that would be silly.
      if (_tcscmp(doc_word, word) == 0)
      {
        continue;
      }

      // Calculate the distance of the words.
      float doc_word_score = this->_stringDistance->getDistance(doc_word, word);
      if (doc_word_score < min)
      {
        continue;
      }

      // Get the frequency of the word.
      //sw->_freq = 0;

      // Maintain the queue of words.
      // The lowest score word is always at the top of the queue.
      if (queue.size() >= numSug)
      {
        // Get the lowest scored word from queue.
        SuggestWordC *lowSw = queue.top();

        // Is it scored lower than the current found one?
        if (lowSw->getScore() < doc_word_score)
        {
          // Remove it from list.
          queue.pop();
          delete lowSw;
          lowSw = 0;

          // Add the found word to queue.
          SuggestWordC *sw = new SuggestWordC;
          sw->setWord(doc_word);
          sw->setScore(doc_word_score);
          queue.push(sw);

          // Define the new minimum distance score by the lowest scored word in queue.
          min = queue.top()->getScore();
          continue;
        }
        else
        {
          // Keep it in list at define the new minimum distance score.
          min = lowSw->getScore();
          continue;
        }
      }
      else
      {
        // Add the word to the queue.
        SuggestWordC *sw = new SuggestWordC;
        sw->setWord(doc_word);
        sw->setScore(doc_word_score);
        queue.push(sw);
      }
    }//for

    // Create return list.
    // Attention: We must fill the list begining from the end, because
    // the queue holds the best result at the end.
    size_t queueLen = queue.size();
    const TCHAR **ret = new const TCHAR*[queueLen+1];

    for (int i=queueLen-1; i>=0; --i)
    {
      // Get the lowest word from queue and remove it.
      SuggestWordC *sw = queue.top();
      queue.pop();

      // Create a new element for the array.
      size_t swWordLen = _tcslen(sw->getWord());
      TCHAR *tmp = new TCHAR[swWordLen+1];
      _tcscpy(tmp, sw->getWord());

      ret[i] = tmp;
      delete sw;
    }
    ret[queueLen] = 0;
    delete hits;
    return ret;
  }

  // Nothing found.
  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Private methods
///////////////////////////////////////////////////////////////////////////////

CL_NS(search)::IndexSearcher* SpellCheckerC::obtainSearcher()
{
  //SCOPED_LOCK_MUTEX(_searchLock);
  return _indexSearcher;
}


void SpellCheckerC::swapSearcher(CL_NS(store)::Directory *dir)
{
  //SCOPED_LOCK_MUTEX(_searchLock);
  CL_NS(search)::IndexSearcher *searcher = createSearcher(dir);
  if (_indexSearcher)
  {
    _indexSearcher->close();
    _CLDELETE(_indexSearcher);
  }
  _indexSearcher = searcher;
}


CL_NS(search)::IndexSearcher* SpellCheckerC::createSearcher(CL_NS(store)::Directory *dir)
{
  return new CL_NS(search)::IndexSearcher(dir);
}


bool SpellCheckerC::exists(const TCHAR *word)
{
  CL_NS(search)::IndexSearcher *searcher = obtainSearcher();
  if (searcher)
  {
    CL_NS(index)::Term t(_fword, word);
    int freq = searcher->docFreq(&t);
    if (freq > 0)
    {
      return true;
    }
  }
  return false;
}


int SpellCheckerC::getMinNGramLen( int wordLen ) const
{
  if( wordLen > 5 )
    return 3;
  if( wordLen == 5 )
    return 2;
  return 1;
}


int SpellCheckerC::getMaxNGramLen( int wordLen ) const
{
  if( wordLen > 5 )
    return 4;
  if( wordLen == 5 )
    return 3;
  return 2;
}


TCHAR** SpellCheckerC::createNGram( const TCHAR *word, int n, int &size ) const
{
  // Abort, if the word is to short for the given value of 'n'.
  int wordLen = _tcslen(word);
  if( wordLen <= n )
    return 0;

  int grammsCount = (wordLen-n)+1;
  TCHAR **gramms = new TCHAR*[grammsCount+1];

  for( int pos=0; pos < grammsCount; pos++ )
  {
    // Get a part of the word.
    // Starting at position 'pos' and then 'n' number of chars.
    TCHAR *tmp = new TCHAR[n+1];

    for( int i=0; i < n; i++ )
      tmp[i] = word[pos+i];
    tmp[n] = 0;

    gramms[pos] = tmp;
  }

  gramms[grammsCount] = 0;
  size = grammsCount;
  return gramms;
}


bool SpellCheckerC::fillDocument( CL_NS(document)::Document *doc, const TCHAR *word ) const
{
  int wordLen = _tcslen( word );
  int min = getMinNGramLen( wordLen );
  int max = getMaxNGramLen( wordLen );
  CL_NS(document)::Field *f = 0;

  // The "word" field.
  f = new CL_NS(document)::Field(
      _fword, word,
      CL_NS(document)::Field::STORE_YES | CL_NS(document)::Field::INDEX_UNTOKENIZED
    );
  doc->add( (*f) );

  // Add grams to document.
  for( int n=min; n<=max; n++ )
  {
    // Convert the current N-gram to wchar_t.
    std::wstringstream ss;
    ss << n;
    std::wstring wsnum = ss.str();
    
    // Fields.    
    std::wstring wsfstart(_T("start"));
    wsfstart += wsnum;

    std::wstring wsfgram(_T("gram"));
    wsfgram += wsnum;

    std::wstring wsfend(_T("end"));
    wsfend += wsnum;

    // Create ngram parts of the search word.
    int grammsLen;
    TCHAR **gramms = createNGram( word, n, grammsLen );

    // The "start(n)" field.
    f = new CL_NS(document)::Field(
        wsfstart.c_str(), gramms[0],
        CL_NS(document)::Field::STORE_NO | CL_NS(document)::Field::INDEX_UNTOKENIZED
      );
    doc->add( (*f) );

    // The "gram(n)" field.
    for( int i=0; i<grammsLen; i++ )
    {
      f = new CL_NS(document)::Field(
          wsfgram.c_str(), gramms[i],
          CL_NS(document)::Field::STORE_NO | CL_NS(document)::Field::INDEX_UNTOKENIZED
        );
      doc->add( (*f) );
    }

    // The "end(n)" field.
    f = new CL_NS(document)::Field(
        wsfend.c_str(), gramms[grammsLen-1],
        CL_NS(document)::Field::STORE_NO | CL_NS(document)::Field::INDEX_UNTOKENIZED
      );
    doc->add( (*f) );

    // Delete gramms.
    _CLDELETE_ARRAY_ALL(gramms);
  }
  return true;
}
