#ifndef _if_wordlistdictionary_header_
#define _if_wordlistdictionary_header_

#include "ApiDef.h"
#include "Dictionary.h"

class CLUCENE_CONTRIBS_EXPORT WordListDictionaryC : public DictionaryC
{
public:
  /**
   * Provides the iteration over multiple words.
   *
   * @param words The words to be iterated. Last element in array must be NULL.
   * @param deleteWords Indicates whether the "words" should be deleted in destructor.
   */
  WordListDictionaryC(const TCHAR **words, bool deleteWords = true);
  ~WordListDictionaryC();

  /**
   * Override methods from DictionaryC
   */
  bool next();
  const TCHAR* word();
  void close();

  /**
   * Sets the internal position to the beginning of the word list.
   */
  void reset();

private:
  const TCHAR **_words;
  int _pos;
  bool _deleteWords;
};

#endif
