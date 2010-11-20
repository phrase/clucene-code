#ifndef _if_spellchecker_header_
#define _if_spellchecker_header_

#include "ApiDef.h"
#include "StringDistance.h"
#include "Dictionary.h"

/**
 * Attention: This class is NOT thread-safe.
 * Only use it within one single thread.
 */
class CLUCENE_CONTRIBS_EXPORT SpellCheckerC
{
public:
  /**
   * Creates a new instance of the SpellCheckerC class.
   *
   * @param dir The directory of the spell index.
   * @param sd The StringDistanceC implementation class which is used to calcuate the string distance between two strings.
   *           If the value is not given, the default LevenshteinDistanceC implementation will be used.
   * @param closeDir Indicates whether the Directory <i>dir</i> should be closed in destructor.
   */
  SpellCheckerC(CL_NS(store)::Directory *dir, StringDistanceC *sd = NULL, bool closeDir = false);
  ~SpellCheckerC();

  /**
   * Sets the directory of the spell index.
   * Basicly the directory should already be set with the constructor.
   *
   * @param dir The destination spell index directory.
   */
  void setSpellIndex(CL_NS(store)::Directory *dir);

  /**
   * Sets the StringDistanceC calculation implementation.
   *
   * @param sd The string distance implementation object.
   */
  void setStringDistance(StringDistanceC *sd);

  /**
   * Set the minimum score value, which a word must have to return as "did you mean" word.
   * Default: 0.5F
   *
   * @param minScore The minimum score, which a word must have (affected by the StringDistanceC implementation)
   */
  void setAccuracy(float minScore);

  /**
   * Creates an spell index based on the "corrected spelled" words provided by the DictionaryC
   * implementation.
   *
   * @param dict Provides words, which should come into the spell index.
   * @param create Indicates, whether the index should be created completely new or the words should be appended to the existing index.
   */
  bool createFromDictionary(DictionaryC &dict, bool create = true);

  /**
   * Searches for suggestions of the given word in the spell index.
   *
   * @param word The word for which the method should provide suggestions.
   * @param numSug The maximum number of suggestions.
   *
   * @return Suggestions for the "word". Array is terminated by NULL.
   */
  const TCHAR** suggestSimilar(const TCHAR *word, int numSug);

private:
  int getMinNGramLen( int wordLen ) const;
  int getMaxNGramLen( int wordLen ) const;
  TCHAR** createNGram( const TCHAR *word, int n, int &size ) const;
  bool fillDocument( CL_NS(document)::Document *doc, const TCHAR *word ) const;
  bool exists(const TCHAR *word);

  CL_NS(search)::IndexSearcher* obtainSearcher();
  void swapSearcher(CL_NS(store)::Directory *dir);
  CL_NS(search)::IndexSearcher* createSearcher(CL_NS(store)::Directory *dir);

private:
  //DEFINE_MUTEX(_thisLock)

  // The fieldname of the word itselfs.
  TCHAR *_fword;

  // Boost values for start- and end-gram.
  float _bStart;
  float _bGram;
  float _bEnd;

  // The minimum score, which the string distance must have.
  float _minScore;

  // The directory of the spell index.
  CL_NS(store)::Directory *_dir;
  bool _closeDir;

  // The string distance algorithm, which is used to find the best matching string.
  StringDistanceC *_stringDistance;

  // The IndexSearch, which is to used to search in the spell index.
  CL_NS(search)::IndexSearcher *_indexSearcher;
};

#endif
