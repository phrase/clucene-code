/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "test.h"
#include "CuTest.h"
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>

static int verbose = 0;
static int messyPrinting = 0;
std::list<LuceneTestCase*> availableTests;

void CuInit(int argc, char *argv[])
{
	int i;
	
	for (i = 0; i < argc; i++) {
		if (!strcmp(argv[i], "-v")) {
			verbose = 1;
		}
		if (!strcmp(argv[i], "-p")) {
			messyPrinting = 1;
		}
	}
}

/*-------------------------------------------------------------------------*
 * CuTcs
 *-------------------------------------------------------------------------*/
TCHAR* CuStrAlloc(int size)
{
	TCHAR* n = (TCHAR*) malloc( sizeof(TCHAR) * (size) );
	return n;
}

TCHAR* CuTcsCopy(TCHAR* old)
{
	int len = _tcslen(old);
	TCHAR* n = CuStrAlloc(len + 1);
	_tcscpy(n, old);
	return n;
}
TCHAR* CuTcsCat(TCHAR* orig, TCHAR* add)
{
	int len = _tcslen(orig) + _tcslen(add);
	TCHAR* n = CuStrAlloc(len + 1);
	_tcscpy(n, orig);
	_tcscat(n, add);
	return n;
}

/*-------------------------------------------------------------------------*
 * CuString
 *-------------------------------------------------------------------------*/

TCHAR* CuTcsAlloc(int size)
{
	TCHAR* n = (TCHAR*) malloc( sizeof(TCHAR) * (size) );
	return n;
}

TCHAR* CuTcsCopy(const TCHAR* old)
{
	int len = _tcslen(old);
	TCHAR* n = CuTcsAlloc(len + 1);
	_tcscpy(n, old);
	return n;
}

/*-------------------------------------------------------------------------*
 * CuString
 *-------------------------------------------------------------------------*/

void CuStringInit(CuString* str)
{
	str->length = 0;
	str->size = STRING_MAX;
	str->buffer = (TCHAR*) malloc(sizeof(TCHAR) * str->size);
	str->buffer[0] = '\0';
}

CuString* CuStringNew(void)
{
	CuString* str = (CuString*) malloc(sizeof(CuString));
	str->length = 0;
	str->size = STRING_MAX;
	str->buffer = (TCHAR*) malloc(sizeof(TCHAR) * str->size);
	str->buffer[0] = '\0';
	return str;
}

void CuStringFree(CuString* str){
	free(str->buffer);
	free(str);
}

void CuStringResize(CuString* str, int newSize)
{
	str->buffer = (TCHAR*) realloc(str->buffer, sizeof(TCHAR) * newSize);
	str->size = newSize;
}

void CuStringAppend(CuString* str, const TCHAR* text)
{
	int length = _tcslen(text);
	if (str->length + length + 1 >= str->size)
		CuStringResize(str, str->length + length + 1 + STRING_INC);
	str->length += length;
	_tcscat(str->buffer, text);
}

void CuStringAppendChar(CuString* str, TCHAR ch)
{
	TCHAR text[2];
	text[0] = ch;
	text[1] = '\0';
	CuStringAppend(str, text);
}

void CuStringAppendFormat(CuString* str, const TCHAR* format, ...)
{
	TCHAR buf[HUGE_STRING_LEN];
	va_list argp;
	va_start(argp, format);
	_vsntprintf(buf, HUGE_STRING_LEN,format, argp);
	va_end(argp);
	CuStringAppend(str, buf);
}

void CuStringRead(CuString *str, TCHAR *path)
{
	path = NULL;
	CU_TDUP(path,str->buffer);
}

/*-------------------------------------------------------------------------*
 * CuTest
 *-------------------------------------------------------------------------*/

void CuTestInit(CuTest* t, const TCHAR* name)
{
	t->name = CuTcsCopy(name);
	t->notimpl = 0;
	t->failed = 0;
	t->ran = 0;
	t->message = NULL;
}

CuTest* CuTestNew(const TCHAR* name)
{
	CuTest* tc = CU_ALLOC(CuTest);
	CuTestInit(tc, name);
	return tc;
}

void CuTestDelete(CuTest* tst){
	free(tst->name);
	if ( tst->message != NULL )
		free(tst->message);
	free(tst);
}

void CuNotImpl(CuTest* tc, const TCHAR* message)
{
	CuString* newstr = CuStringNew();
    CuStringAppend(newstr, message);
    CuStringAppend(newstr, _T(" not implemented on this platform"));
	tc->notimpl = 1;
	CuMessage(tc,newstr->buffer);
	CuStringFree(newstr);
//	if (tc->jumpBuf != 0) longjmp(*(tc->jumpBuf), 0);
}

void CuFail(CuTest* tc, const TCHAR* format, ...)
{
	tc->failed = 1;

	TCHAR buf[HUGE_STRING_LEN];
	va_list argp;
	va_start(argp, format);
	_vsntprintf(buf, HUGE_STRING_LEN, format, argp);
	va_end(argp);

//	CuMessage(tc,buf);
	_CLTHROWT(CL_ERR_Runtime, buf);
}

void CuFail(CuTest* tc, CLuceneError& e)
{
	tc->failed = 1;
	throw e;
}

void CuMessageV(CuTest* tc, const TCHAR* format, va_list& argp){
	TCHAR buf[HUGE_STRING_LEN];
	_vsntprintf(buf, HUGE_STRING_LEN, format, argp);

	TCHAR* old = tc->message;
	if ( messyPrinting ){
      _tprintf(_T("%s"),buf);	   
	}else{
   	if ( old == NULL ){
   		tc->message = CuTcsCopy(buf);
   	}else{
   		tc->message = CuTcsCat(old,buf);
   		free(old);
   	}
   }
}
void CuMessage(CuTest* tc, const TCHAR* format, ...){
	va_list argp;
	va_start(argp, format);
	CuMessageV(tc,format,argp);
	va_end(argp);
}
void CuMessageA(CuTest* tc, const char* format, ...){
	va_list argp;
	char buf[HUGE_STRING_LEN];
	TCHAR tbuf[HUGE_STRING_LEN];
	va_start(argp, format);
	vsprintf(buf, format, argp);
	va_end(argp);

	TCHAR* old = tc->message;
	STRCPY_AtoT(tbuf,buf,HUGE_STRING_LEN);
	if ( messyPrinting ){
      _tprintf(_T("%s"),buf);	   
	}else{
   	if ( old == NULL ){
   		tc->message = CuTcsCopy(tbuf);
   	}else{
   		tc->message = CuTcsCat(old,tbuf);
   		free(old);
   	}
   }
}

void CuAssert(CuTest* tc, const TCHAR* message, int condition)
{
	if (condition) return;
	CuFail(tc, message);
}

void CuAssertTrue(CuTest* tc, int condition, const TCHAR* msg)
{
	if (condition) return;
    if (msg != NULL)
	    CuFail(tc, msg);
    else
        CuFail(tc, _T("assert failed"));
}

void CuAssertEquals(CuTest* tc, const int32_t expected, const int32_t actual, const TCHAR* msg)
{
    CuAssertIntEquals(tc, msg, expected, actual);
}

void CuAssertStrEquals(CuTest* tc, const TCHAR* preMessage, const TCHAR* expected, TCHAR* actual, bool bDelActual){
  CuString* message;
  if (_tcscmp(expected, actual) == 0) {
    if (bDelActual) _CLDELETE_LCARRAY(actual);
    return;
  }
  message = CuStringNew();
  if (preMessage) {
    CuStringAppend(message, preMessage);
    CuStringAppend(message, _T(" : ") );
  }
  CuStringAppend(message, _T("expected\n---->\n"));
  CuStringAppend(message, expected);
  CuStringAppend(message, _T("\n<----\nbut saw\n---->\n"));
  CuStringAppend(message, actual);
  if (bDelActual) _CLDELETE_LCARRAY(actual);
  CuStringAppend(message, _T("\n<----"));
  CuFail(tc, message->buffer);
  CuStringFree(message);
}
void CuAssertStrEquals(CuTest* tc, const TCHAR* preMessage, const TCHAR* expected, const TCHAR* actual)
{
	CuString* message;
	if (_tcscmp(expected, actual) == 0) {
		return;
	}
	message = CuStringNew();
	if (preMessage) {
		CuStringAppend(message, preMessage);
		CuStringAppend(message, _T(" : ") );
	}
	CuStringAppend(message, _T("expected\n---->\n"));
	CuStringAppend(message, expected);
	CuStringAppend(message, _T("\n<----\nbut saw\n---->\n"));
	CuStringAppend(message, actual);
	CuStringAppend(message, _T("\n<----"));
	CuFail(tc, message->buffer);
	CuStringFree(message);
}

void CuAssertIntEquals(CuTest* tc, const TCHAR* preMessage, int expected, int actual)
{
    if (expected == actual) return;
    TCHAR buf[STRING_MAX];
    if (preMessage != NULL){
        _sntprintf(buf, STRING_MAX, _T("%s : expected <%d> but was <%d>"), preMessage, expected, actual);
    } else {
        _sntprintf(buf, STRING_MAX, _T("Assert failed : expected <%d> but was <%d>"), expected, actual);
    }
    CuFail(tc, buf);
}
void CuAssertSizeEquals(CuTest* tc, const TCHAR* preMessage, size_t expected, size_t actual)
{
    if (expected == actual) return;
    TCHAR buf[STRING_MAX];
    if (preMessage != NULL){
        _sntprintf(buf, STRING_MAX, _T("%s : expected <%d> but was <%d>"), preMessage, expected, actual);
    } else {
        _sntprintf(buf, STRING_MAX, _T("Assert failed : expected <%d> but was <%d>"), expected, actual);
    }
    CuFail(tc, buf);
}

void CuAssertPtrEquals(CuTest* tc, const TCHAR* preMessage, const void* expected, const void* actual)
{
	TCHAR buf[STRING_MAX];
	if (expected == actual) return;
	_sntprintf(buf, STRING_MAX,_T("%s : expected pointer <%p> but was <%p>"), preMessage, expected, actual);
	CuFail(tc, buf);
}

void CuAssertPtrNotNull(CuTest* tc, const TCHAR* preMessage, const void* pointer)
{
	TCHAR buf[STRING_MAX];
	if (pointer != NULL ) return;
	_sntprintf(buf,STRING_MAX, _T("%s : null pointer unexpected, but was <%p>"), preMessage, pointer);
	CuFail(tc, buf);
}


/*-------------------------------------------------------------------------*
 * CuSuite
 *-------------------------------------------------------------------------*/

void CuReportProgress(CuTest* test)
{
	if (test->failed) printf("F");
	else if (test->notimpl) printf("N");
	else printf(".");
	fflush(stdout);
}
/*
void CuSuiteSummary(CuSuite* testSuite, CuString* summary, bool times)
{
	int i;
	for (i = 0 ; i < testSuite->count ; ++i)
	{
		CuTest* testCase = testSuite->list[i];
		CuStringAppend(summary, testCase->failed ? _T("F") : 
                               testCase->notimpl ? _T("N"): _T("."));
	}
	if ( times ){
		int bufferLen = 25-summary->length-10;
		for (int i=0;i<bufferLen;i++ )
			CuStringAppend(summary,_T(" "));
		CuStringAppendFormat(summary,_T(" - %dms"), (int32_t)testSuite->timeTaken);
	}
	CuStringAppend(summary, _T("\n"));
}

void CuSuiteOverView(CuSuite* testSuite, CuString* details)
{
	CuStringAppendFormat(details, _T("%d %s run:  %d passed, %d failed, ")
			     _T("%d not implemented.\n"),
			     testSuite->count, 
			     testSuite->count == 1 ? "test" : "tests",
   			     testSuite->count - testSuite->failCount - 
				testSuite->notimplCount,
			     testSuite->failCount, testSuite->notimplCount);
}*/

void CuSuiteDetails(LuceneTestCase* test, CuString* details)
{
	list<CuTest*>::iterator it;
	int failCount = 0;

	if (test->FailCount() > 0 && verbose)
	{
		CuStringAppendFormat(details, _T("\nFailed tests in %s:\n"), test->GetName());
        
        for ( it=test->testsRan.begin() ; it != test->testsRan.end(); it++ )
		{
			if ((*it)->failed)
			{
				failCount++;
				CuStringAppendFormat(details, _T("%d) %s: %s\n"), 
					failCount, (*it)->name, (*it)->message);
			}
		}
	}

	if (test->NotImplCount() > 0 && verbose)
	{
		CuStringAppendFormat(details, _T("\nNot Implemented tests in %s:\n"), test->GetName());
		for ( it=test->testsRan.begin() ; it != test->testsRan.end(); it++ )
		{
			if ((*it)->notimpl)
			{
			        failCount++;
			        CuStringAppendFormat(details, _T("%d) %s: %s\n"),
			                failCount, (*it)->name, (*it)->message);
			}
		}
	}
}

void CuSuiteListRun(list<LuceneTestCase*>& tests)
{
	list<LuceneTestCase*>::iterator it;
    for ( it=tests.begin() ; it != tests.end(); it++ )
	{
        (*it)->RunTests();
	}
}

static const char *genspaces(int i)
{
    char *str = (char*)malloc((i + 1) * sizeof(char));
    for ( int j=0;j<i;j++ )
		str[j]=' ';
    str[i] = '\0';
    return str;
}

void CuSuiteListRunWithSummary(list<LuceneTestCase*>& tests, bool verbose, bool times)
{
	//_tprintf(_T("%s:\n"), testSuite->name);
    
    list<LuceneTestCase*>::iterator it;
    for ( it=tests.begin() ; it != tests.end(); it++ )
	{
		bool hasprinted=false;
		CuString *str = CuStringNew();

		size_t len = strlen((*it)->GetName());
		const char* spaces = len>31?NULL:genspaces(31 - len);
		printf("    %s:%s", (*it)->GetName(), len>31?"":spaces);
		free((void*)spaces);
		fflush(stdout);
		
		(*it)->RunTests();
        if (times){
            // TODO: Some spacing
            //int bufferLen = 25-summary->length-10;
            //for (int i=0;i<bufferLen;i++ ) CuStringAppend(summary,_T(" "));
            printf(" - %dms\n", (int32_t)(*it)->timeTaken);
        }

		if ( verbose ){
            list<CuTest*>::iterator itSingleTest;
            for ( itSingleTest=(*it)->testsRan.begin() ; itSingleTest != (*it)->testsRan.end(); itSingleTest++ )
            {
                if ( (*itSingleTest)->ran ){
					if ( (*itSingleTest)->message != NULL ){
						if ( !hasprinted )
							printf("\n");
						_tprintf(_T("      %s:\n"),(*itSingleTest)->name);

						TCHAR* msg = (*itSingleTest)->message;
						bool nl = true;
						//write out message, indenting on new lines
						while ( *msg != '\0' ){
							if ( nl ){
								printf("        ");
								nl=false;
							}
							if ( *msg == '\n' )
								nl = true;
							putc(*msg,stdout);

							msg++;
						}

						if ( (*itSingleTest)->message[_tcslen((*itSingleTest)->message)-1] != '\n' )
							printf("\n");
						hasprinted=true;
					}
				}
			}
        }
		//CuSuiteSummary((*it), str, times);
		if ( hasprinted )
			_tprintf(_T("    Result: %s\n"), str->buffer);
		else
			_tprintf(_T(" %s"), str->buffer);

		CuStringFree(str);
	}
	printf("\n");
}

int CuSuiteListDetails(list<LuceneTestCase*>& tests, CuString* details)
{
	unsigned int failCount = 0;
	unsigned int notImplCount = 0;
	unsigned int count = 0;

	list<LuceneTestCase*>::iterator it;
    for ( it=tests.begin() ; it != tests.end(); it++ )
	{
        failCount += (*it)->FailCount();
        notImplCount += (*it)->NotImplCount();
        count += (*it)->TotalCount();
	}

	CuStringAppendFormat(details, _T("%d %s run:  %d passed, %d failed, ")
			     _T("%d not implemented.\n"),
			     count, 
			     count == 1 ? _T("test") : _T("tests"),
   			     count - failCount - notImplCount,
			     failCount, notImplCount);

	if (failCount != 0 && verbose)
	{
        for ( it=tests.begin() ; it != tests.end(); it++ )
        {
			CuString *str = CuStringNew();
            if ((*it)->FailCount())
			{
				CuSuiteDetails((*it), str);
				CuStringAppend(details, str->buffer);
			}
			CuStringFree(str);
		}
	}
	if (notImplCount != 0 && verbose)
	{
		for ( it=tests.begin() ; it != tests.end(); it++ )
		{
			CuString *str = CuStringNew();
			if ((*it)->NotImplCount())
			{
				CuSuiteDetails((*it), str);
				CuStringAppend(details, str->buffer);
			}
			CuStringFree(str);
		}
	} 
	return failCount;
}

