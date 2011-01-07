#include "test.h"
#include <stdio.h>
#include "CLucene/spellchecker/WordListDictionary.h"
#include "CLucene/spellchecker/SpellChecker.h"


void testSpellIndexSearch(CuTest *tc)
{
  char fsdir[CL_MAX_PATH];
  _snprintf(fsdir, CL_MAX_PATH, "%s/%s", cl_tempDir, "test.spellindex");
  Directory *fsDir = FSDirectory::getDirectory(fsdir);

  SpellCheckerC sc(fsDir, 0, true);
  const TCHAR **suggs = sc.suggestSimilar(_T("insune"), 1);

  if (suggs != NULL) {
    CuAssertStrEquals(tc, _T(""), _T("insane"), suggs[0]);
  } else {
    CuFail(tc, _T("Suggestions expected"));
  }
}

CuSuite *testspellchecksearch(void)
{
  CuSuite *suite = CuSuiteNew(_T("CLucene SpellCheck Search Test"));
  SUITE_ADD_TEST(suite, testSpellIndexSearch);

  return suite;
}
