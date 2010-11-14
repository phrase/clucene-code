#ifndef _if_suggestword_header_
#define _if_suggestword_header_

#include "ApiDef.h"

/**
 * This class is only used for internal priority list of search in SpellCheckerC
 */
class /*CLUCENE_CONTRIBS_EXPORT*/ SuggestWordC
{
public:
  SuggestWordC();
  ~SuggestWordC();

  void setWord(const TCHAR *word);
  const TCHAR* getWord() const;

  void setScore(float score);
  float getScore() const;

  void setFreq(int freq);
  int getFreq() const;

  bool operator<(SuggestWordC *sw);
  bool operator<(SuggestWordC &sw);
  bool operator>(SuggestWordC *sw);
  bool operator>(SuggestWordC &sw);
  bool operator()(SuggestWordC &sw1, SuggestWordC &sw2);
  bool operator()(SuggestWordC *sw1, SuggestWordC *sw2);

public:
  float _score;
  int   _freq;
  TCHAR *_word;
};

#endif
