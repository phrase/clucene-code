/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/shared_ptr.hpp>
#include "_ConjunctionScorer.h"
#include "CLucene/index/Term.h"
#include "Similarity.h"
#include "CLucene/util/_Arrays.h"
#include <assert.h>
#include <algorithm>

CL_NS_USE(index)
CL_NS_USE(util)
CL_NS_DEF(search)

	ConjunctionScorer::ConjunctionScorer(Similarity* similarity, Scorer::Vector& _scorers):
		Scorer(similarity),
		firstTime(true),
		more(false),
		coord(0.0),
		lastDoc(-1)
	{
		scorers.transfer(scorers.end(), _scorers.begin(), _scorers.end(), _scorers);
		coord = getSimilarity()->coord(scorers.size(), scorers.size());
	}
	/*
  ConjunctionScorer::ConjunctionScorer(Similarity* similarity, const Scorer::Array& _scorers):
		Scorer(similarity),
		firstTime(true),
		more(false),
		coord(0.0),
		lastDoc(-1)
	{
    this->scorers.reset(new CL_NS(util)::ObjectArray<Scorer>(_scorers->length));
    memcpy(this->scorers->values, _scorers->values, _scorers->length * sizeof(Scorer*));
    coord = getSimilarity()->coord(this->scorers->length, this->scorers->length);
  }
	*/
	ConjunctionScorer::~ConjunctionScorer(){
		// empty
	}

	TCHAR* ConjunctionScorer::toString(){
		return stringDuplicate(_T("ConjunctionScorer"));
	}
	
  int32_t ConjunctionScorer::doc()  const{ 
    return lastDoc;
  }

  bool ConjunctionScorer::next()  {
    if (firstTime) {
      init(0);
    } else if (more) {
      more = scorers[scorers.size() - 1].next();
    }
    return doNext();
  }

  bool ConjunctionScorer::doNext() {
    int32_t first=0;
    Scorer* lastScorer = &(scorers[scorers.size() - 1]);
    Scorer* firstScorer;
    while (more && (firstScorer=&(scorers[first]))->doc() < (lastDoc=lastScorer->doc())) {
      more = firstScorer->skipTo(lastDoc);
      lastScorer = firstScorer;
      first = (first == (scorers.size() - 1)) ? 0 : first + 1;
    }
    return more;
  }

  bool ConjunctionScorer::skipTo(int32_t target) {
    if (firstTime)
      return init(target);
    else if (more)
      more = scorers[scorers.size() - 1].skipTo(target);
    return doNext();
  }
  int ConjunctionScorer_sort(const void* _elem1, const void* _elem2){
    const Scorer* elem1 = *(const Scorer**)_elem1;
    const Scorer* elem2 = *(const Scorer**)_elem2;
	  return elem1->doc() - elem2->doc();
  }

  bool ConjunctionScorer::init(int32_t target)  {
    firstTime = false;
    more = scorers.size() > 1;
    
    for (size_t i = 0; i < scorers.size(); i++) {
      more = target == 0 ? scorers[i].next() : scorers[i].skipTo(target);
      if (!more)
        return false;
    }

    // Sort the array the first time...
    // We don't need to sort the array in any future calls because we know
    // it will already start off sorted (all scorers on same doc).

    // note that this comparator is not consistent with equals!
		scorers.sort();

    doNext();

    // If first-time skip distance is any predictor of
    // scorer sparseness, then we should always try to skip first on
    // those scorers.
    // Keep last scorer in it's last place (it will be the first
    // to be skipped on), but reverse all of the others so that
    // they will be skipped on in order of original high skip.
    int32_t end = (scorers.size() - 1) - 1;
		Scorer::Vector::iterator beginIt = scorers.begin();
		Scorer::Vector::iterator endIt = scorers.begin();
    for (int32_t i=0; i<(end>>1); i++) {
      Scorer::Vector::auto_type tmp = scorers.replace(beginIt, scorers.release(endIt).release());
      scorers.replace(endIt, tmp.release());
			beginIt++;
			endIt--;
    }
    return more;
  }

  float_t ConjunctionScorer::score(){
    float_t sum = 0.0f;
    for (size_t i = 0; i < scorers.size(); i++) {
      sum += scorers[i].score();
    }
    return sum * coord;
  }
  Explanation* ConjunctionScorer::explain(int32_t doc) {
	  _CLTHROWA(CL_ERR_UnsupportedOperation,"UnsupportedOperationException: ConjunctionScorer::explain");
  }


CL_NS_END
