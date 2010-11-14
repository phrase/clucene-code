#ifndef _if_indexdictionary_header_
#define _if_indexdictionary_header_

#include "ApiDef.h"
#include "Dictionary.h"

class CLUCENE_CONTRIBS_EXPORT IndexDictionaryC : public DictionaryC
{
public:
  /**
   * Creates a dictionary for an existing full text index and makes it possible to iterate
   * through the terms of an index.
   *
   * @param dir - The full text index directory.
   * @param fieldName - If this value is not NULL, only the terms of this field will be indexed.
   * @param closeDir - Indicates whether the given Directory instance should be closed and deleted.
   */
  IndexDictionaryC(CL_NS(store)::Directory *dir, const TCHAR *fieldName=NULL, bool closeDir=false);
  ~IndexDictionaryC();

  /**
   * Finds out whether the Dictionary has a next word.
   */
  bool next();

  /**
   * Gets the current term text of the full text index.
   * The function does not return a copy, the word will be deleted by IndexDictionaryC.
   */
  const TCHAR* word();

  /**
   * Cleans up the Dictionary. (This function is called by destructor, too)
   */
  void close();

private:
  CL_NS(index)::TermEnum* getTermEnum();

private:
  const TCHAR *_fieldName;
  CL_NS(store)::Directory *_dir;
  bool _closeDir;

  CL_NS(index)::IndexReader *_indexReader;
  CL_NS(index)::TermEnum *_termEnum;
  CL_NS(index)::Term *_term;
};

#endif
