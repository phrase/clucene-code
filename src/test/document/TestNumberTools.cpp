/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "test.h"

    void subtestTwoLongs(CuTest *tc, int64_t i, int64_t j) {
        // convert to strings
		std::basic_string<TCHAR> a = NumberTools::longToString(i);
		std::basic_string<TCHAR> b = NumberTools::longToString(j);

        // are they the right length?
		CuAssertTrue(tc, NumberTools::STR_SIZE == _tcslen(a.c_str()));
		CuAssertTrue(tc, NumberTools::STR_SIZE == _tcslen(b.c_str()));

        // are they the right order?
        if (i < j) {
            CuAssertTrue(tc, _tcscmp(a.c_str(),b.c_str()) < 0);
        } else if (i > j) {
            CuAssertTrue(tc, _tcscmp(a.c_str(),b.c_str()) > 0);
        } else {
            CuAssertTrue(tc, _tcscmp(a.c_str(),b.c_str()) == 0);
        }

        // can we convert them back to longs?
		int64_t i2 = NumberTools::stringToLong(a);
		int64_t j2 = NumberTools::stringToLong(b);

        CuAssertTrue(tc, i == i2);
        CuAssertTrue(tc, j == j2);
    }

    void testNearZero(CuTest *tc) {
        for (int32_t i = -100; i <= 100; i++) {
            for (int32_t j = -100; j <= 100; j++) {
                subtestTwoLongs(tc, i, j);
            }
        }
    }

    void testMin(CuTest *tc) {
        // make sure the constants convert to their equivelents
		CuAssertTrue(tc, LUCENE_INT64_MIN_SHOULDBE == NumberTools::stringToLong(const_cast<TCHAR*>(NumberTools::MIN_STRING_VALUE)));
		std::basic_string<TCHAR> actual = NumberTools::longToString(LUCENE_INT64_MIN_SHOULDBE);
		CuAssertStrEquals(tc, _T("Min value"), NumberTools::MIN_STRING_VALUE, actual.c_str());

        // test near MIN, too
        for (int64_t l = LUCENE_INT64_MIN_SHOULDBE; l < LUCENE_INT64_MIN_SHOULDBE + 10000; l++) {
            subtestTwoLongs(tc,l, l + 1);
        }
    }

    void testMax(CuTest *tc) {
        // make sure the constants convert to their equivelents
		CuAssertTrue(tc, LUCENE_INT64_MAX_SHOULDBE == NumberTools::stringToLong(const_cast<TCHAR*>(NumberTools::MAX_STRING_VALUE)));
		basic_string<TCHAR> actual = NumberTools::longToString(LUCENE_INT64_MAX_SHOULDBE);
		CuAssertStrEquals(tc, _T("Max value"), NumberTools::MAX_STRING_VALUE, actual.c_str());

        // test near MAX, too
        for (int64_t l = LUCENE_INT64_MAX_SHOULDBE; l > LUCENE_INT64_MAX_SHOULDBE - 10000; l--) {
            subtestTwoLongs(tc,l, l - 1);
        }
    }

CuSuite *testNumberTools(void)
{
	CuSuite *suite = CuSuiteNew(_T("CLucene Number Tools Test"));

	SUITE_ADD_TEST(suite, testNearZero);
	SUITE_ADD_TEST(suite, testMin);
	SUITE_ADD_TEST(suite, testMax);
    return suite; 
}
