#include "IndexDictionary.h"

IndexDictionaryC::IndexDictionaryC( CL_NS(store)::Directory *dir, const TCHAR *fieldName, bool closeDir )
:_dir(dir), _fieldName(fieldName), _closeDir(closeDir), _term(NULL), _termEnum(NULL), _indexReader(NULL)
{
  // Create the IndexReader.
  this->_indexReader = CL_NS(index)::IndexReader::open(this->_dir);
  // TODO: Copy fieldName parameter.
}

IndexDictionaryC::~IndexDictionaryC()
{
  this->close();
}

bool IndexDictionaryC::next()
{
  CL_NS(index)::TermEnum *tEnum = this->getTermEnum();
  if (tEnum == NULL)
  {
    return false;
  }

  // Here we have got a valid enumeration.
  this->_term = tEnum->term();
  if (this->_term == NULL)
  {
    return false; // End of enumeration.
  }

  tEnum->next();
  return true;
}

const TCHAR* IndexDictionaryC::word()
{
  if (this->_term != NULL)
  {
    return this->_term->text();
  }
  return NULL;
}

void IndexDictionaryC::close()
{
  if (this->_indexReader != NULL)
  {
    this->_indexReader->close();
    delete this->_indexReader;
  }

  if (this->_closeDir && this->_dir != NULL)
  {
    this->_dir->close();
    _CLDECDELETE(this->_dir);
  }

  if (this->_fieldName != NULL)
  {
    delete this->_fieldName;
  }
}

CL_NS(index)::TermEnum* IndexDictionaryC::getTermEnum()
{
  if (this->_termEnum == NULL)
  {
    if (this->_fieldName != NULL)
    {
      CL_NS(index)::Term *startTerm = new CL_NS(index)::Term(this->_fieldName, L"");
      this->_termEnum = this->_indexReader->terms(startTerm);
      _CLDECDELETE(startTerm);
    }
    else
    {
      this->_termEnum = this->_indexReader->terms();
    }

    // Do we have a valid enumeration?
    if (this->_termEnum->term(false) == NULL)
    {// No!
      _CLDELETE(this->_termEnum);
      return NULL;
    }
  }
  return this->_termEnum;
}
