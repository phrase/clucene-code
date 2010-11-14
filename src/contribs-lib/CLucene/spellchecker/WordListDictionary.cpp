#include "WordListDictionary.h"

WordListDictionaryC::WordListDictionaryC(const TCHAR **words, bool deleteWords)
:_words(words), _deleteWords(deleteWords), _pos(-1)
{
}


WordListDictionaryC::~WordListDictionaryC()
{
  close();
}


bool WordListDictionaryC::next()
{
  if (this->_words[this->_pos+1] != 0)
  {
    this->_pos++;
    return true;
  }
  return false;
}


const TCHAR* WordListDictionaryC::word()
{
  return this->_words[this->_pos];
}


void WordListDictionaryC::reset()
{
  this->_pos = -1;
}


void WordListDictionaryC::close()
{
  if (this->_deleteWords)
  {
    _CLDELETE_ARRAY_ALL(this->_words);
  }
}
