#ifndef _if_dictionary_header_
#define _if_dictionary_header_

#include "ApiDef.h"

class CLUCENE_CONTRIBS_EXPORT DictionaryC
{
public:
  virtual bool next() = 0;
  virtual const TCHAR* word() = 0;
  virtual void close() = 0;
};

#endif
