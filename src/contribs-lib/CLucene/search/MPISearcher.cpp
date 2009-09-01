/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* Copyright (C) 2009 Isidor Zeuner
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "CLucene/index/IndexReader.h"
#include "MPISearcher.h"
#include "CLucene/search/SearchHeader.h"
#include "CLucene/search/Query.h"
#include "CLucene/search/_HitQueue.h"
#include "CLucene/document/Document.h"
#include "CLucene/index/Term.h"
#include "CLucene/search/_FieldDocSortedHitQueue.h"
#include <boost/serialization/vector.hpp>
#include <boost/serialization/deque.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/variant.hpp>

CL_NS_USE(index)
CL_NS_USE(util)
CL_NS_USE(document)

CL_NS_DEF(search)

    class deallocating_char : public Compare::Char {
        char* value;
    public:
        deallocating_char(char* value) : Compare::Char(value), value(value) {
        }
        ~deallocating_char() {
            free(value);
        }
    };

    class deallocating_wchar : public Compare::WChar {
        wchar_t* value;
    public:
        deallocating_wchar(wchar_t* value) : Compare::WChar(value), value(value) {
        }
        ~deallocating_wchar() {
            free(value);
        }
    };

    class comparable_visitor : public boost::static_visitor<Comparable*> {
    public:
        Comparable* operator()(int32_t serialized) const {
            return _CLNEW Compare::Int32(serialized);
        }
        Comparable* operator()(float_t serialized) const {
            return _CLNEW Compare::Float(serialized);
        }
        Comparable* operator()(std::pair<bool, std::string> const& serialized) const {
            return _CLNEW deallocating_char(
                serialized.first ? strdup(serialized.second.c_str()) : NULL
            );
        }
        Comparable* operator()(std::pair<bool, std::wstring> const& serialized) const {
            return _CLNEW deallocating_wchar(
                serialized.first ? wcsdup(serialized.second.c_str()) : NULL
            );
        }
    };


    class MPIHitCollector: public HitCollector{
    private:
      boost::mpi::communicator world;
      HitCollector* results;
      int32_t start;
      typedef std::pair<int32_t, float_t> score_doc;
      typedef std::deque<score_doc> doc_queue;
      doc_queue hits;
    public: 
      MPIHitCollector(boost::mpi::communicator world, HitCollector* _results, int32_t _start);
      void collect(const int32_t doc, const float_t score) ;
      void pass_up();
    };
    

  /** Creates a searcher which searches <i>searchers</i>. */
  MPISearcher::MPISearcher(boost::mpi::communicator world, Searchable** _searchables):
		world(world) {
    searchables = _searchables[world.rank()];
    int32_t local_maxDoc = searchables->maxDoc();
    searchablesLen = 0;
    while ( _searchables[searchablesLen] != NULL ) {
        if (world.rank() != searchablesLen) {
            _searchables[searchablesLen]->close();
        }
        ++searchablesLen;
    }

    if (searchablesLen > world.size()) {
        _CLTHROWA(CL_ERR_Runtime, "world too small");
    }

    if (searchablesLen < world.size()) {
        _CLTHROWA(CL_ERR_Runtime, "world too big");
    }

    starts = _CL_NEWARRAY(int32_t, searchablesLen + 1);

    if (0 == world.rank()) {

        starts[0] = 0;

        gather(world, local_maxDoc, starts + 1, 0);

        for (int processing = 1; processing < searchablesLen; processing++) {
            starts[processing + 1] += starts[processing];
        }

        _maxDoc = starts[searchablesLen];

        for (int processing = 1; processing < searchablesLen; processing++) {
            for (int offset = 0; offset < 2; offset++) {
                world.send(processing, offset, starts[processing + offset]);
            }
        }
    } else {
        gather(world, local_maxDoc, 0);

        for (int offset = 0; offset < 2; offset++) {
            world.recv(0, offset, starts[world.rank() + offset]);
        }
    }
  }

  MPISearcher::~MPISearcher() {
    _CLDELETE_ARRAY(starts);
  }

	int32_t* MPISearcher::getStarts() {
		return starts;
	}
	int32_t MPISearcher::getLength() {
		return searchablesLen;
	}

  // inherit javadoc
  void MPISearcher::close() {
    searchables->close();
    searchables=NULL;
  }

  int32_t MPISearcher::docFreq(boost::shared_ptr<const Term> const& term) const {
    int32_t local_docFreq = searchables->docFreq(term);
    int32_t docFreq;
    if (0 == world.rank()) {
      reduce(world, local_docFreq, docFreq, std::plus<int32_t>(), 0);
    } else {
      reduce(world, local_docFreq, std::plus<int32_t>(), 0);
    }
    return docFreq;
  }

  /** For use by {@link HitCollector} implementations. */
  bool MPISearcher::doc(int32_t n, Document* d) {
    typedef std::vector<
      std::pair<
        std::pair<
          std::basic_string<TCHAR>,
          std::basic_string<TCHAR>
        >,
        int32_t
      >
    > serializable_document_data;
    int local_success = 0;
    if (selfSubSearcher(n)) {
      local_success = searchables->doc(n - starts[world.rank()], d) ? 1 : 0;
    }
    if (0 == world.rank()) {
      int success;
      reduce(world, local_success, success, boost::mpi::maximum<int>(), 0);
      if (0 == success) {
        return false;
      }
      if (selfSubSearcher(n)) {
        return true;
      }
      serializable_document_data document_data;
      world.recv(boost::mpi::any_source, 0, document_data);
      size_t fields = document_data.size();
      for (size_t processed = 0; processed < fields; processed++) {
        d->add(
          *_CLNEW CL_NS(document)::Field(
            document_data[processed].first.first.c_str(),
            document_data[processed].first.second.c_str(),
            document_data[processed].second,
            true
          )
        );
      }
      return true;
    } else {
      reduce(world, local_success, boost::mpi::maximum<int>(), 0);
      if (selfSubSearcher(n)) {
        size_t fields = d->getFields()->size();
        serializable_document_data document_data(fields);
        for (size_t processed = 0; processed < fields; processed++) {
          document_data[processed].first.first = (*(d->getFields()))[processed]->name();
          document_data[processed].first.second = (*(d->getFields()))[processed]->stringValue();
          int32_t config = 0;
          if ((*(d->getFields()))[processed]->isStored()) {
            config += CL_NS(document)::Field::STORE_YES;
          } else {
            config += CL_NS(document)::Field::STORE_NO;
          }
          if ((*(d->getFields()))[processed]->isIndexed()) {
            if ((*(d->getFields()))[processed]->isTokenized()) {
              config += CL_NS(document)::Field::INDEX_TOKENIZED;
            } else {
              config += CL_NS(document)::Field::INDEX_UNTOKENIZED;
            }
          } else {
            config += CL_NS(document)::Field::INDEX_NO;
          }
          document_data[processed].second = config;
        }
        world.send(0, 0, document_data);
      }
    }
  }

  int32_t MPISearcher::searcherIndex(int32_t n) const{
	 return subSearcher(n);
  }

  bool MPISearcher::selfSubSearcher(int32_t n) const {
    return (starts[world.rank()] <= n) && (n < starts[world.rank() + 1]);
  }

  std::vector<MPISearcher::comparable_value> MPISearcher::packed_comparables(
      Comparable** unpacked
  ) {
      size_t length = 0;
      for (; unpacked[length] != NULL; length++);
      std::vector<comparable_value> result(length);
      for (size_t packing = 0; packing < result.size(); packing++) {
          const char* class_name = unpacked[packing]->getObjectName();
          if (Compare::Int32::getClassName() == class_name) {
              result[packing] = static_cast<Compare::Int32*>(unpacked[packing])->getValue();
          } else if (Compare::Float::getClassName() == class_name) {
              result[packing] = static_cast<Compare::Float*>(unpacked[packing])->getValue();
          } else if (Compare::Char::getClassName() == class_name) {
              if (NULL == static_cast<Compare::Char*>(unpacked[packing])->getValue()) {
                  result[packing] = std::pair<bool, std::string>(false, "");
              } else {
                  result[packing] = std::pair<bool, std::string>(
                      true,
                      static_cast<Compare::Char*>(unpacked[packing])->getValue()
                  );
              }
          } else if (Compare::WChar::getClassName() == class_name) {
              if (NULL == static_cast<Compare::WChar*>(unpacked[packing])->getValue()) {
                  result[packing] = std::pair<bool, std::wstring>(false, L"");
              } else {
                  result[packing] = std::pair<bool, std::wstring>(
                      true,
                      static_cast<Compare::WChar*>(unpacked[packing])->getValue()
                  );
              }
          } else {
              _CLTHROWA(CL_ERR_Runtime, "unknown comparable");
          }
      }
      return result;
  }

  Comparable** MPISearcher::unpacked_comparables(
      std::vector<MPISearcher::comparable_value> const& packed
  ) {
      Comparable** result = _CL_NEWARRAY(Comparable*, packed.size() + 1);
      for (size_t packing = 0; packing < packed.size(); packing++) {
          result[packing] = boost::apply_visitor(comparable_visitor(), packed[packing]);
      }
      result[packed.size()] = NULL;
      return result;
  }

  /** Returns index of the searcher for document <code>n</code> in the array
   * used to construct this searcher. */
  int32_t MPISearcher::subSearcher(int32_t n) const{
    if (0 == world.rank()) {
      if (selfSubSearcher(n)) {
        return world.rank();
      } else {
        int32_t result;
        world.recv(boost::mpi::any_source, 0, result);
        return result;
      }
    } else {
      if (selfSubSearcher(n)) {
        world.send(0, 0, world.rank());
      }
    }
  }

  /** Returns the document number of document <code>n</code> within its
   * sub-index. */
  int32_t MPISearcher::subDoc(int32_t n)  const{
    if (0 == world.rank()) {
      if (selfSubSearcher(n)) {
        return n - starts[world.rank()];
      } else {
        int32_t result;
        world.recv(boost::mpi::any_source, 0, result);
        return result;
      }
    } else {
      if (selfSubSearcher(n)) {
        world.send(0, 0, n - starts[world.rank()]);
      }
    }
  }

  int32_t MPISearcher::maxDoc() const{
    return _maxDoc;
  }

  TopDocs* MPISearcher::_search(Query* query, Filter* filter, const int32_t nDocs) {
    HitQueue* hq = _CLNEW HitQueue(nDocs);
    int32_t totalHits = 0;
    TopDocs* docs;
    int32_t j;
    ScoreDoc* scoreDocs;

    docs = searchables->_search(query, filter, nDocs);
    scoreDocs = docs->scoreDocs;
    typedef std::vector<std::pair<int32_t, float_t> > serializable_document_data;
    serializable_document_data document_data(docs->scoreDocsLength);
    std::vector<serializable_document_data> all_document_data;
    for ( j = 0; j <docs->scoreDocsLength; ++j) {
      document_data[j].first = scoreDocs[j].doc + starts[world.rank()];
      document_data[j].second = scoreDocs[j].score;
    }
    for (int32_t gathering = 0; gathering < searchablesLen; gathering++) {
      if (gathering == world.rank()) {
        reduce(world, docs->totalHits, totalHits, std::plus<int32_t>(), gathering);
        gather(world, document_data, all_document_data, gathering);
        for (int32_t i = 0; i < all_document_data.size(); i++) {
          for (j = 0; j < all_document_data[i].size(); j++) {
            ScoreDoc deserialized;
            deserialized.doc = all_document_data[i][j].first;
            deserialized.score = all_document_data[i][j].second;
            if (!hq->insert(deserialized)) {
              break;
            }
          }
        }
      } else {
        reduce(world, docs->totalHits, std::plus<int32_t>(), gathering);
        gather(world, document_data, gathering);
      }
    }
    _CLDELETE(docs);

    int32_t scoreDocsLen = hq->size();
		scoreDocs = new ScoreDoc[scoreDocsLen];
	{//MSVC 6 scope fix
		for (int32_t i = scoreDocsLen-1; i >= 0; --i)	  // put docs in array
	  		scoreDocs[i] = hq->pop();
	}

	//cleanup
	_CLDELETE(hq);

    return _CLNEW TopDocs(totalHits, scoreDocs, scoreDocsLen);
  }

  /** Lower-level search API.
   *
   * <p>{@link HitCollector#collect(int32_t,float_t)} is called for every non-zero
   * scoring document.
   *
   * <p>Applications should only use this if they need <i>all</i> of the
   * matching documents.  The high-level search API ({@link
   * Searcher#search(Query)}) is usually more efficient, as it skips
   * non-high-scoring hits.
   *
   * @param query to match documents
   * @param filter if non-null, a bitset used to eliminate some documents
   * @param results to receive hits
   */
  void MPISearcher::_search(Query* query, Filter* filter, HitCollector* results){
      /* DSR:CL_BUG: Old implementation leaked and was misconceived.  We need
      ** to have the original HitCollector ($results) collect *all* hits;
      ** the MPIHitCollector instantiated below serves only to adjust
      ** (forward by starts[i]) the docNo passed to $results.
      ** Old implementation instead created a sort of linked list of
      ** MPIHitCollectors that applied the adjustments in $starts
      ** cumulatively (and was never deleted). */
      MPIHitCollector *docNoAdjuster = _CLNEW MPIHitCollector(world, results, starts[world.rank()]);
      searchables->_search(query, filter, docNoAdjuster);
      docNoAdjuster->pass_up();
      _CLDELETE(docNoAdjuster);
  }

  TopFieldDocs* MPISearcher::_search (Query* query, Filter* filter, const int32_t n, const Sort* sort){
    FieldDocSortedHitQueue* hq = NULL;
    int32_t totalHits = 0;
    TopFieldDocs* docs;
    int32_t j;
    FieldDoc** fieldDocs;

    docs = searchables->_search(query, filter, n, sort);
    hq = _CLNEW FieldDocSortedHitQueue (docs->fields, n);
    docs->fields = NULL; //hit queue takes fields memory

    fieldDocs = docs->fieldDocs;

    typedef std::vector<
      std::pair<
        std::pair<int32_t, float_t>,
        std::vector<comparable_value>
      >
    > serializable_document_data;
    serializable_document_data document_data(docs->scoreDocsLength);
    std::vector<serializable_document_data> all_document_data;
    for ( j = 0; j <docs->scoreDocsLength; ++j) {
      document_data[j].first.first = fieldDocs[j]->scoreDoc.doc + starts[world.rank()];
      document_data[j].first.second = fieldDocs[j]->scoreDoc.score;
      document_data[j].second = packed_comparables(fieldDocs[j]->fields);
    }
    for (int32_t gathering = 0; gathering < searchablesLen; gathering++) {
      if (gathering == world.rank()) {
        reduce(world, docs->totalHits, totalHits, std::plus<int32_t>(), gathering);
        gather(world, document_data, all_document_data, gathering);
      } else {
        reduce(world, docs->totalHits, std::plus<int32_t>(), gathering);
        gather(world, document_data, gathering);
      }
    }
    for (int32_t i = 0; i < all_document_data.size(); i++) {
      for (j = 0; j < all_document_data[i].size(); j++) {
        Comparable** fields = unpacked_comparables(all_document_data[i][j].second);
        FieldDoc* deserialized = _CLNEW FieldDoc(
          all_document_data[i][j].first.first,
          all_document_data[i][j].first.second,
          fields
        );
        if (!hq->insert(deserialized)) {
          break;
        }
      }
    }
    _CLDELETE(docs);

    int32_t hqlen = hq->size();
	fieldDocs = _CL_NEWARRAY(FieldDoc*,hqlen);
	for (j = hqlen - 1; j >= 0; j--)	  // put docs in array
      fieldDocs[j] = hq->pop();

	SortField** hqFields = hq->getFields();
	hq->setFields(NULL); //move ownership of memory over to TopFieldDocs
    _CLDELETE(hq);

    return _CLNEW TopFieldDocs (totalHits, fieldDocs, hqlen, hqFields);
  }

  Query* MPISearcher::rewrite(Query* query) {
    // this is a bit of a hack. We know that a query which
    // creates a Weight based on this Dummy-Searcher is
    // always already rewritten (see preparedWeight()).
    // Therefore we just return the unmodified query here
    return query;
  }

  void MPISearcher::explain(Query* query, int32_t doc, Explanation* ret) {
    // TODO: send explanation to master
    if (selfSubSearcher(doc)) {
      searchables->explain(query,doc-starts[world.rank()], ret); // dispatch to searcher
    }
  }

  MPIHitCollector::MPIHitCollector(boost::mpi::communicator world, HitCollector* _results, int32_t _start):
  world(world),
  results(_results),
	start(_start) {
  }

  void MPIHitCollector::collect(const int32_t doc, const float_t score) {
    hits.push_back(std::pair<int32_t, float_t>(doc + start, score));
  }

  void MPIHitCollector::pass_up() {
    if (0 == world.rank()) {
      std::vector<doc_queue> all_queues;
      gather(world, hits, all_queues, 0);
      for (size_t processed = 0; processed < all_queues.size(); processed++) {
        doc_queue& passing = all_queues[processed];
        while (!passing.empty()) {
          score_doc& collected = passing.front();
          results->collect(collected.first, collected.second);
          passing.pop_front();
        }
      }
    } else {
      gather(world, hits, 0);
    }
  }

CL_NS_END
