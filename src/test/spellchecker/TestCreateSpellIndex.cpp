#include "test.h"
#include <stdio.h>
#include "CLucene/spellchecker/WordListDictionary.h"
#include "CLucene/spellchecker/IndexDictionary.h"
#include "CLucene/spellchecker/SpellChecker.h"

void testSpellIndexCreation(CuTest *tc)
{
  // create some words for the index.
  const TCHAR **words = new const TCHAR*[9];
  words[0] = L"insane";
  words[1] = L"factory";
  words[2] = L"administrator";
  words[3] = L"development";
  words[4] = L"windows";
  words[5] = L"linux";
  words[6] = L"professional";
  words[7] = L"software";
  words[8] = 0;

  // create a dictionary from the words array.
  WordListDictionaryC dict(words, false);

	char fsdir[CL_MAX_PATH];
	_snprintf(fsdir, CL_MAX_PATH, "%s/%s", cl_tempDir, "test.spellindex");

  // file system directory.
  Directory *fsDir = FSDirectory::getDirectory(fsdir);

  // create spell checker.
  SpellCheckerC sc(fsDir, 0, true);
  bool created = sc.createFromDictionary(dict, true);

  CLUCENE_ASSERT(created);
}

void testSpellIndexCreationFromIndex(CuTest *tc)
{
  // Step 1: Create index.
	char fsdir[CL_MAX_PATH];
	_snprintf(fsdir, CL_MAX_PATH, "%s/%s", cl_tempDir, "test.spellindex-lucene");
  Directory *fsDir = FSDirectory::getDirectory(fsdir);

  CL_NS(analysis)::SimpleAnalyzer analyzer;
  CL_NS(index)::IndexWriter writer(fsDir, &analyzer, true, true);

  Document doc;
  doc.add(
      *new Field(_T("data"), _T("insane factory administrator development windows linux professional software"),
      Field::STORE_YES | Field::INDEX_TOKENIZED)
    );

  writer.addDocument(&doc);
  writer.optimize();
  writer.close();

  // Step 2: Create spell index from lucene index.
	char scdir[CL_MAX_PATH];
	_snprintf(scdir, CL_MAX_PATH, "%s/%s", cl_tempDir, "test.spellindex-from-lucene");
  Directory *scDir = FSDirectory::getDirectory(scdir);

  IndexDictionaryC dict(fsDir, _T("data"), true);
  SpellCheckerC spellChecker(scDir, 0, true);
  bool created = spellChecker.createFromDictionary(dict, true);
  CLUCENE_ASSERT(created);
}

CuSuite *testspellcheckcreation(void)
{
	CuSuite *suite = CuSuiteNew(_T("CLucene SpellCheck Creation Test"));
	SUITE_ADD_TEST(suite, testSpellIndexCreation);
  SUITE_ADD_TEST(suite, testSpellIndexCreationFromIndex);

  return suite;
}
