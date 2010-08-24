#include "test.h"

LuceneTestCase::LuceneTestCase(const char* _testName) : failCount(0), notImplCount(0), timeTaken(0)
{
    testName = strdup(_testName);
    availableTests.push_back(this);
}

const char* LuceneTestCase::GetName() const
{
    return testName;
}

unsigned int LuceneTestCase::FailCount() const
{
    return failCount;
}

unsigned int LuceneTestCase::NotImplCount() const
{
    return notImplCount;
}

unsigned int LuceneTestCase::TotalCount() const
{
    return testsRan.size();
}

LuceneTestCase::~LuceneTestCase()
{
    delete[] testName;
    
    list<CuTest*>::iterator it;
    for ( it=testsRan.begin() ; it != testsRan.end(); it++ )
        CuTestDelete(*it);
}