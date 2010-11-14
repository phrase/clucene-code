#include "SuggestWord.h"

SuggestWordC::SuggestWordC()
:_score(0.0F)
,_freq(0)
,_word(0)
{
}

SuggestWordC::~SuggestWordC()
{
  try
  {
    if(_word)
    {
      delete _word;
    }
  }
  catch(...)
  {
  }
}

void SuggestWordC::setWord(const TCHAR *word)
{
  if (word)
  {
    int wordLen = _tcslen(word);
    if (wordLen > 0)
    {
      _word = new TCHAR[wordLen+1];
      _tcscpy(_word, word);
    }
  }
}

const TCHAR* SuggestWordC::getWord() const
{
  return _word;
}

void SuggestWordC::setScore(float score)
{
  _score = score;
}

float SuggestWordC::getScore() const
{
  return _score;
}

void SuggestWordC::setFreq(int freq)
{
  _freq = freq;
}

int SuggestWordC::getFreq() const
{
  return _freq;
}

bool SuggestWordC::operator<(SuggestWordC *sw)
{
  return this->_score < sw->_score;
}

bool SuggestWordC::operator<(SuggestWordC &sw)
{
  return this->_score < sw._score;
}

bool SuggestWordC::operator>(SuggestWordC *sw)
{
  return this->_score > sw->_score;
}

bool SuggestWordC::operator>(SuggestWordC &sw)
{
  return this->_score > sw._score;
}

bool SuggestWordC::operator()(SuggestWordC &sw1, SuggestWordC &sw2)
{
  return sw1._score > sw2._score;
}

bool SuggestWordC::operator()(SuggestWordC *sw1, SuggestWordC *sw2)
{
  return sw1->_score > sw2->_score;
}
