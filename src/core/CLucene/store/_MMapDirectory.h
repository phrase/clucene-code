/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_store_MMapDirectory_
#define _lucene_store_MMapDirectory_

#include "CLucene/util/VoidMap.h"
#include "FSDirectory.h"
#include "IndexInput.h"

CL_NS_DEF(store)

	/**
	* Experimental:
  * A raw string {@link Directory} implementation. Not compatible
  * with JLucene. Should be faster, because no utf-8-esque encoding and
  * decoding needs to be done.
  * NOTE: There is no detection of format, so if you write with this,
  * then opening with a normal FSDirectory may have unpredictable results
  * (and vice-versa)
	*
	*/
	class CLUCENE_EXPORT MMapDirectory:public FSDirectory{
  private:
	  class MMapIndexInput: public IndexInput{
		  class Internal;
		  Internal* _internal;

		  MMapIndexInput(const MMapIndexInput& clone);
	  public:
		  MMapIndexInput(const char* path);
		  virtual ~MMapIndexInput();
		  IndexInput* clone() const;

		  inline uint8_t readByte();
		  int32_t readVInt();
		  void readBytes(uint8_t* b, const int32_t len);
		  void close();
		  int64_t getFilePointer() const;
		  void seek(const int64_t pos);
		  int64_t length() const;

		  const char* getDirectoryType() const{ return MMapDirectory::getClassName(); }
      const char* getObjectName() const{ return getClassName(); }
      static const char* getClassName() { return "MMapIndexInput"; }	
	  };
    
	  class MultiMMapIndexInput: public IndexInput{
		  class Internal;
		  Internal* _internal;
      
		  MultiMMapIndexInput(const MultiMMapIndexInput& clone);
	  public:
		  MultiMMapIndexInput(const char* path, int maxBufSize);
		  virtual ~MultiMMapIndexInput();
		  IndexInput* clone() const;

		  inline uint8_t readByte();
		  void readBytes(uint8_t* b, const int32_t len);
		  void close();
		  int64_t getFilePointer() const;
		  void seek(const int64_t pos);
		  int64_t length() const;

		  const char* getDirectoryType() const{ return MultiMMapIndexInput::getClassName(); }
      const char* getObjectName() const{ return getClassName(); }
      static const char* getClassName() { return "MultiMMapIndexInput"; }	
	  };
	public:
		MMapDirectory(const char* path, const bool createDir, LockFactory* lockFactory=NULL);

    ///Destructor - only call this if you are sure the directory
	  ///is not being used anymore. Otherwise use the ref-counting
	  ///facilities of _CLDECDELETE
		virtual ~MMapDirectory();

		/// Returns a stream reading an existing file.
		virtual bool openInput(const char* name, IndexInput*& ret, CLuceneError& err, int32_t bufferSize=-1);

		static const char* getClassName();
		const char* getObjectName() const;
	};
CL_NS_END
#endif
