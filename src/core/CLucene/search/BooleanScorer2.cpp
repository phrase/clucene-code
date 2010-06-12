/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/shared_ptr.hpp>
#include "_BooleanScorer2.h"

#include "Scorer.h"
#include "SearchHeader.h"
#include "CLucene/index/Term.h"
#include "Similarity.h"
#include "ScorerDocQueue.h"
#include "Explanation.h"

#include "_BooleanScorer.h"
#include "_BooleanScorer.h"
#include "_ConjunctionScorer.h"
#include "_DisjunctionSumScorer.h"

CL_NS_USE(util)
CL_NS_DEF(search)


class BooleanScorer2::Coordinator {
public:

	/** Auto pointer for Coordinator. */
	typedef std::auto_ptr<Coordinator> AutoPtr;

	int32_t maxCoord;
	Scorer::Vector::size_type nrMatchers; // to be increased by score() of match counting scorers.
	float_t* coordFactors;
	Scorer* parentScorer;

	Coordinator( Scorer* parent ):
		maxCoord(0),
		nrMatchers(0),
		coordFactors(NULL),
		parentScorer(parent)
	{
	}

	virtual ~Coordinator()
	{
		_CLDELETE_ARRAY(coordFactors);
	}

	void init()
	{
		coordFactors = _CL_NEWARRAY( float_t, maxCoord+1 );
		Similarity* sim = parentScorer->getSimilarity();
		for ( int32_t i = 0; i <= maxCoord; i++ ) {
			coordFactors[i] = sim->coord(i, maxCoord);
		}
	}


	void initDoc() {
		nrMatchers = 0;
	}

	float_t coordFactor() {
		return coordFactors[nrMatchers];
	}
};

class BooleanScorer2::SingleMatchScorer: public Scorer {
public:
	const Scorer::AutoPtr scorer;
	Coordinator* coordinator;
	int32_t lastScoredDoc;

	SingleMatchScorer(Scorer::AutoPtr _scorer, Coordinator* _coordinator) :
    Scorer( _scorer->getSimilarity() ), scorer(_scorer), coordinator(_coordinator), lastScoredDoc(-1)
	{
	}

	virtual ~SingleMatchScorer()
	{
		// empty
	}

	float_t score()
	{
		if ( doc() >= lastScoredDoc ) {
			lastScoredDoc = this->doc();
			coordinator->nrMatchers++;
		}
		return scorer->score();
	}

	int32_t doc() const {
		return scorer->doc();
	}

	bool next() {
		return scorer->next();
	}

	bool skipTo( int32_t docNr ) {
		return scorer->skipTo( docNr );
	}

	virtual TCHAR* toString() {
		return scorer->toString();
	}

	Explanation* explain(int32_t doc) {
		return scorer->explain( doc );
	}

};

/** A scorer that matches no document at all. */
class BooleanScorer2::NonMatchingScorer: public Scorer {
public:

	NonMatchingScorer() :
		Scorer( NULL )
	{
	}
	virtual ~NonMatchingScorer() {};

	int32_t doc() const {
		_CLTHROWA(CL_ERR_UnsupportedOperation, "UnsupportedOperationException: BooleanScorer2::NonMatchingScorer::doc");
		return 0;
	}
	bool next() { return false; }
	float_t score() {
		_CLTHROWA(CL_ERR_UnsupportedOperation, "UnsupportedOperationException: BooleanScorer2::NonMatchingScorer::score");
		return 0.0;
	}
	bool skipTo( int32_t target ) { return false; }
	virtual TCHAR* toString() { return stringDuplicate(_T("NonMatchingScorer")); }

	Explanation* explain( int32_t doc ) {
		Explanation* e = _CLNEW Explanation();
		e->setDescription(_T("No document matches."));
		return e;
	}

};

/** A Scorer for queries with a required part and an optional part.
 * Delays skipTo() on the optional part until a score() is needed.
 * <br>
 * This <code>Scorer</code> implements {@link Scorer#skipTo(int)}.
 */
class BooleanScorer2::ReqOptSumScorer: public Scorer {
private:
	/** The scorers passed from the constructor.
	* These are set to null as soon as their next() or skipTo() returns false.
	*/
	Scorer::AutoPtr reqScorer;
	Scorer::AutoPtr optScorer;
	bool firstTimeOptScorer;

public:
	/** Construct a <code>ReqOptScorer</code>.
	* @param reqScorer The required scorer. This must match.
	* @param optScorer The optional scorer. This is used for scoring only.
	*/
	ReqOptSumScorer(Scorer::AutoPtr _reqScorer, Scorer::AutoPtr _optScorer) :
      Scorer( NULL ), reqScorer(_reqScorer), optScorer(_optScorer), firstTimeOptScorer(true)
	{
	}

	virtual ~ReqOptSumScorer()
	{
		// empty
	}

	/** Returns the score of the current document matching the query.
	* Initially invalid, until {@link #next()} is called the first time.
	* @return The score of the required scorer, eventually increased by the score
	* of the optional scorer when it also matches the current document.
	*/
	float_t score()
	{
		int32_t curDoc = reqScorer->doc();
		float_t reqScore = reqScorer->score();

		if ( firstTimeOptScorer ) {
			firstTimeOptScorer = false;
			if ( !optScorer->skipTo( curDoc ) ) {
				optScorer.reset();
				return reqScore;
			}
		} else if ( optScorer.get() == NULL ) {
			return reqScore;
		} else if (( optScorer->doc() < curDoc ) && ( !optScorer->skipTo( curDoc ))) {
			optScorer.reset();
			return reqScore;
		}

		return ( optScorer->doc() == curDoc )
			? reqScore + optScorer->score()
			: reqScore;
	}

	int32_t doc() const {
		return reqScorer->doc();
	}

	bool next() {
		return reqScorer->next();
	}

	bool skipTo( int32_t target ) {
		return reqScorer->skipTo( target );
	}

	virtual TCHAR* toString() {
		return stringDuplicate(_T("ReqOptSumScorer"));
	}

	/** Explain the score of a document.
	* @todo Also show the total score.
	* See BooleanScorer.explain() on how to do this.
	*/
	Explanation* explain( int32_t doc ) {
		Explanation* res = _CLNEW Explanation();
		res->setDescription(_T("required, optional"));
		res->addDetail(reqScorer->explain(doc));
		res->addDetail(optScorer->explain(doc));
		return res;
	}
};


/** A Scorer for queries with a required subscorer and an excluding (prohibited) subscorer.
* <br>
* This <code>Scorer</code> implements {@link Scorer#skipTo(int)},
* and it uses the skipTo() on the given scorers.
*/
class BooleanScorer2::ReqExclScorer: public Scorer {
private:
	Scorer::AutoPtr reqScorer;
	Scorer::AutoPtr exclScorer;
	bool firstTime;

public:
	/** Construct a <code>ReqExclScorer</code>.
	* @param reqScorer The scorer that must match, except where
	* @param exclScorer indicates exclusion.
	*/
	ReqExclScorer( Scorer::AutoPtr _reqScorer, Scorer::AutoPtr _exclScorer ) :
      Scorer( NULL ), reqScorer(_reqScorer), exclScorer(_exclScorer), firstTime(true)
	{
	}

	virtual ~ReqExclScorer()
	{
		// empty
	}

	int32_t doc() const {
		return reqScorer->doc();
	}

	/** Returns the score of the current document matching the query.
	* Initially invalid, until {@link #next()} is called the first time.
	* @return The score of the required scorer.
	*/
	float_t score() {
		return reqScorer->score();
	}

	virtual TCHAR* toString() {
		return stringDuplicate(_T("ReqExclScorer"));
	}

	Explanation* explain( int32_t doc ) {
		Explanation* res = _CLNEW Explanation();
		if (exclScorer->skipTo(doc) && (exclScorer->doc() == doc)) {
			res->setDescription(_T("excluded"));
		} else {
			res->setDescription(_T("not excluded"));
			res->addDetail(reqScorer->explain(doc));
		}
		return res;
	}


	bool next()
	{
		if ( firstTime ) {
			if ( !exclScorer->next() ) {
				exclScorer.reset();
			}
			firstTime = false;
		}
		if ( reqScorer.get() == NULL ) {
			return false;
		}
		if ( !reqScorer->next() ) {
			reqScorer.reset(); // exhausted, nothing left
			return false;
		}
		if ( exclScorer.get() == NULL ) {
			return true;// reqScorer.next() already returned true
		}
		return toNonExcluded();
	}


	/** Skips to the first match beyond the current whose document number is
	* greater than or equal to a given target.
	* <br>When this method is used the {@link #explain(int)} method should not be used.
	* @param target The target document number.
	* @return true iff there is such a match.
	*/
	bool skipTo( int32_t target )
	{
		if ( firstTime ) {
			firstTime = false;
			if ( !exclScorer->skipTo( target )) {
				exclScorer.reset(); // exhausted
			}
		}
		if ( reqScorer.get() == NULL ) {
			return false;
		}
		if ( exclScorer.get() == NULL ) {
			return reqScorer->skipTo( target );
		}
		if ( !reqScorer->skipTo( target )) {
			reqScorer.reset();
			return false;
		}
		return toNonExcluded();
	}


private:
	/** Advance to non excluded doc.
	* <br>On entry:
	* <ul>
	* <li>reqScorer != null,
	* <li>exclScorer != null,
	* <li>reqScorer was advanced once via next() or skipTo()
	*      and reqScorer.doc() may still be excluded.
	* </ul>
	* Advances reqScorer a non excluded required doc, if any.
	* @return true iff there is a non excluded required doc.
	*/
	bool toNonExcluded()
	{
		int32_t exclDoc = exclScorer->doc();

		do {
			int32_t reqDoc = reqScorer->doc();
			if ( reqDoc < exclDoc ) {
				return true; // reqScorer advanced to before exclScorer, ie. not excluded
			} else if ( reqDoc > exclDoc ) {
				if (! exclScorer->skipTo(reqDoc)) {
					exclScorer.reset(); // exhausted, no more exclusions
					return true;
				}
			  exclDoc = exclScorer->doc();
			  if ( exclDoc > reqDoc ) {
				  return true; // not excluded
			  }
			}
		} while ( reqScorer->next() );

		reqScorer.reset(); // exhausted, nothing left
		return false;
	}

};

class BooleanScorer2::BSConjunctionScorer: public CL_NS(search)::ConjunctionScorer {
private:
	CL_NS(search)::BooleanScorer2::Coordinator* coordinator;
	int32_t lastScoredDoc;
	Scorer::Vector::size_type requiredNrMatchers;
public:
	BSConjunctionScorer( CL_NS(search)::BooleanScorer2::Coordinator* _coordinator, Scorer::Vector& _requiredScorers, Scorer::Vector::size_type _requiredNrMatchers ):
		ConjunctionScorer( Similarity::getDefault(), _requiredScorers ),
		coordinator(_coordinator),
		lastScoredDoc(-1),
		requiredNrMatchers(_requiredNrMatchers)
	{
	}

	virtual ~BSConjunctionScorer(){
	}
	float_t score()
	{
		if ( this->doc() >= lastScoredDoc ) {
			lastScoredDoc = this->doc();
			coordinator->nrMatchers += requiredNrMatchers;
		}
		return ConjunctionScorer::score();
	}
	virtual TCHAR* toString() {return stringDuplicate(_T("BSConjunctionScorer"));}
};

class BooleanScorer2::BSDisjunctionSumScorer: public CL_NS(search)::DisjunctionSumScorer {
private:
	CL_NS(search)::BooleanScorer2::Coordinator* coordinator;
	int32_t lastScoredDoc;
public:
	BSDisjunctionSumScorer(
		CL_NS(search)::BooleanScorer2::Coordinator* _coordinator,
		Scorer::Vector& subScorers,
		Scorer::Vector::size_type minimumNrMatchers ):
			DisjunctionSumScorer( subScorers, minimumNrMatchers ),
			coordinator(_coordinator),
			lastScoredDoc(-1)
	{
	}

	float_t score() {
		if ( this->doc() >= lastScoredDoc ) {
			lastScoredDoc = this->doc();
			coordinator->nrMatchers += _nrMatchers;
		}
		return DisjunctionSumScorer::score();
	}

	virtual ~BSDisjunctionSumScorer(){
	}
	virtual TCHAR* toString() {return stringDuplicate(_T("BSDisjunctionSumScorer"));}
};

class BooleanScorer2::Internal{
public:

	Scorer::Vector requiredScorers;
	Scorer::Vector optionalScorers;
	Scorer::Vector prohibitedScorers;

	const BooleanScorer2::Coordinator::AutoPtr coordinator;
	Scorer::AutoPtr countingSumScorer;

	size_t minNrShouldMatch;
	bool allowDocsOutOfOrder;


	void initCountingSumScorer()
	{
		coordinator->init();
		countingSumScorer = makeCountingSumScorer();
	}

	Scorer::AutoPtr countingDisjunctionSumScorer(Scorer::Vector& scorers, Scorer::Vector::size_type minNrShouldMatch)
	{
		Scorer::AutoPtr result(new BSDisjunctionSumScorer(coordinator.get(), scorers, minNrShouldMatch));
		return result;
	}

	Scorer::AutoPtr countingConjunctionSumScorer(Scorer::Vector& requiredScorers)
	{
		Scorer::AutoPtr result(new BSConjunctionScorer(coordinator.get(), requiredScorers, requiredScorers.size()));
		return result;
	}

	Scorer::AutoPtr dualConjunctionSumScorer(Scorer::AutoPtr req1, Scorer::AutoPtr req2)
	{
		Scorer::Vector scorers;
		scorers.push_back(req1);
		scorers.push_back(req2);
		Scorer::AutoPtr result(new ConjunctionScorer(Similarity::getDefault(), scorers));
		return result;
	}

	Scorer::AutoPtr makeCountingSumScorer()
	{
		return (requiredScorers.size() == 0)
			? makeCountingSumScorerNoReq()
			: makeCountingSumScorerSomeReq();
	}

	Scorer::AutoPtr makeCountingSumScorerNoReq()
	{
		if (optionalScorers.size() == 0) {
			Scorer::AutoPtr result(new NonMatchingScorer());
			return result;
		} else {
			size_t nrOptRequired = ( minNrShouldMatch < 1 ) ? 1 : minNrShouldMatch;
			if ( optionalScorers.size() < nrOptRequired ) {
				Scorer::AutoPtr result(new NonMatchingScorer());
				return result;
			} else {
				Scorer::AutoPtr requiredCountingSumScorer;

				if (optionalScorers.size() > nrOptRequired) {
					requiredCountingSumScorer = countingDisjunctionSumScorer(optionalScorers, nrOptRequired);
				} else if (optionalScorers.size() == 1) {
					Scorer::AutoPtr firstScorer(optionalScorers.release(optionalScorers.begin()).release());
					requiredCountingSumScorer.reset(new SingleMatchScorer(firstScorer, coordinator.get()));
				} else {
					requiredCountingSumScorer = countingConjunctionSumScorer(optionalScorers);
				}

				return addProhibitedScorers(requiredCountingSumScorer);
			}
		}
	}

	Scorer::AutoPtr makeCountingSumScorerSomeReq()
	{
		if ( optionalScorers.size() < minNrShouldMatch ) {
			Scorer::AutoPtr result(new NonMatchingScorer());
			return result;
		} else if ( optionalScorers.size() == minNrShouldMatch ) {
			Scorer::Vector allReq;
			allReq.transfer(allReq.end(), requiredScorers.begin(), requiredScorers.end(), requiredScorers);
			allReq.transfer(allReq.end(), optionalScorers.begin(), optionalScorers.end(), requiredScorers);
			return addProhibitedScorers(countingConjunctionSumScorer(allReq));
		} else {
			Scorer::AutoPtr requiredCountingSumScorer;

			if (requiredScorers.size() == 1) {
				Scorer::AutoPtr firstScorer(requiredScorers.release(requiredScorers.begin()).release());
				requiredCountingSumScorer.reset(new SingleMatchScorer(firstScorer, coordinator.get()));
			} else {
				requiredCountingSumScorer = countingConjunctionSumScorer(requiredScorers);
			}

			if ( minNrShouldMatch > 0 ) {
				return addProhibitedScorers(
					dualConjunctionSumScorer(
							requiredCountingSumScorer,
							countingDisjunctionSumScorer(
								optionalScorers,
								minNrShouldMatch )));
			} else {
				Scorer::AutoPtr subScorer;

				if (optionalScorers.size() == 1) {
					Scorer::AutoPtr firstScorer(optionalScorers.release(optionalScorers.begin()).release());
					subScorer.reset(new SingleMatchScorer(firstScorer, coordinator.get()));
				} else {
					subScorer = countingDisjunctionSumScorer(optionalScorers, 1);
				}

				Scorer::AutoPtr result(new ReqOptSumScorer(
					addProhibitedScorers(requiredCountingSumScorer), subScorer));
				return result;
			}
		}
	}

	Scorer::AutoPtr addProhibitedScorers( Scorer::AutoPtr requiredCountingSumScorer )
	{
		if (prohibitedScorers.size() == 0) {
			return requiredCountingSumScorer;
		} else {
			Scorer::AutoPtr subScorer;

			if (prohibitedScorers.size() == 1) {
				subScorer.reset(prohibitedScorers.release(prohibitedScorers.begin()).release());
			} else {
				subScorer.reset(new DisjunctionSumScorer(prohibitedScorers));
			}

			Scorer::AutoPtr result(new ReqExclScorer(requiredCountingSumScorer, subScorer));
			return result;
		}
	}

	Internal( BooleanScorer2* parent, int32_t _minNrShouldMatch, bool _allowDocsOutOfOrder ):
		requiredScorers(false),
		optionalScorers(false),
		prohibitedScorers(false),
		coordinator(new Coordinator(parent)),
		minNrShouldMatch(_minNrShouldMatch),
		allowDocsOutOfOrder(_allowDocsOutOfOrder)
	{
		if ( minNrShouldMatch < 0 ) {
      _CLTHROWA(CL_ERR_IllegalArgument, "Minimum number of optional scorers should not be negative");
		}
	}
	~Internal() {
		// empty
	}

};

BooleanScorer2::BooleanScorer2( Similarity* similarity, int32_t minNrShouldMatch, bool allowDocsOutOfOrder ):
	Scorer( similarity )
{
	_internal.reset(new Internal(this, minNrShouldMatch, allowDocsOutOfOrder));
}

BooleanScorer2::~BooleanScorer2()
{
	// empty
}

void BooleanScorer2::add( Scorer::AutoPtr scorer, bool required, bool prohibited )
{
	if ( !prohibited ) {
		_internal->coordinator->maxCoord++;
	}

	if ( required ) {
		if ( prohibited ) {
			_CLTHROWA(CL_ERR_IllegalArgument, "scorer cannot be required and prohibited");
		}
		_internal->requiredScorers.push_back(scorer);
	} else if ( prohibited ) {
		_internal->prohibitedScorers.push_back(scorer);
	} else {
		_internal->optionalScorers.push_back(scorer);
	}
}

void BooleanScorer2::score( HitCollector* hc )
{
	if ( _internal->allowDocsOutOfOrder && _internal->requiredScorers.size() == 0 && _internal->prohibitedScorers.size() < 32 ) {

		const BooleanScorer::AutoPtr bs(new BooleanScorer( getSimilarity(), _internal->minNrShouldMatch ));

		// Ownership is transfered from optionalScorers to bs
		Scorer::Vector::iterator si = _internal->optionalScorers.begin();
		while ( si != _internal->optionalScorers.end() ) {
			Scorer::AutoPtr scorer(_internal->optionalScorers.release(si).release());
			bs->add(scorer, false /* required */, false /* prohibited */ );
			si++;
		}

		// Ownership is transfered from probibitedScorers to bs
		si = _internal->prohibitedScorers.begin();
		while ( si != _internal->prohibitedScorers.begin() ) {
			Scorer::AutoPtr scorer(_internal->prohibitedScorers.release(si).release());
			bs->add(scorer, false /* required */, true /* prohibited */ );
		}
		bs->score( hc );
	} else {
		if ( _internal->countingSumScorer.get() == NULL ) {
			_internal->initCountingSumScorer();
		}
		while ( _internal->countingSumScorer->next() ) {
			hc->collect( _internal->countingSumScorer->doc(), score() );
		}
	}
}

int32_t BooleanScorer2::doc() const
{
	return _internal->countingSumScorer->doc();
}

bool BooleanScorer2::next()
{
	if ( _internal->countingSumScorer.get() == NULL ) {
		_internal->initCountingSumScorer();
	}
	return _internal->countingSumScorer->next();
}

float_t BooleanScorer2::score()
{
	_internal->coordinator->initDoc();
	float_t sum = _internal->countingSumScorer->score();
	return sum * _internal->coordinator->coordFactor();
}

bool BooleanScorer2::skipTo( int32_t target )
{
	if ( _internal->countingSumScorer.get() == NULL ) {
		_internal->initCountingSumScorer();
	}
	return _internal->countingSumScorer->skipTo( target );
}

TCHAR* BooleanScorer2::toString()
{
	return stringDuplicate(_T("BooleanScorer2"));
}

Explanation* BooleanScorer2::explain( int32_t doc )
{
	_CLTHROWA(CL_ERR_UnsupportedOperation,"UnsupportedOperationException: BooleanScorer2::explain");
	/* How to explain the coordination factor?
	initCountingSumScorer();
	return countingSumScorer.explain(doc); // misses coord factor.
	*/
}

bool BooleanScorer2::score( HitCollector* hc, int32_t max )
{
	int32_t docNr = _internal->countingSumScorer->doc();
	while ( docNr < max ) {
		hc->collect( docNr, score() );
		if ( !_internal->countingSumScorer->next() ) {
			return false;
		}
		docNr = _internal->countingSumScorer->doc();
	}
	return true;
}
CL_NS_END
