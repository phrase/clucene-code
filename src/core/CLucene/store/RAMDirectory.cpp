/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "RAMDirectory.h"
#include "_RAMDirectory.h"
#include "Lock.h"
#include "LockFactory.h"
#include "Directory.h"
#include "FSDirectory.h"
#include "BufferedIndexOutput.h"
#include "CLucene/index/IndexReader.h"
//#include "CLucene/util/VoidMap.h"
#include "CLucene/util/Misc.h"
#include <assert.h>

CL_NS_USE(util)
CL_NS_DEF(store)


	// *****
	// Lock acquisition sequence:  RAMDirectory, then RAMFile
	// *****

	class RAMLock: public LuceneLock{
	private:
		RAMDirectory* directory;
		char* fname;
	public:
		RAMLock(const char* name, RAMDirectory* dir);
		virtual ~RAMLock();
		bool obtain();
		void release();
		bool isLocked();
		virtual std::string toString();
	};



  RAMFile::RAMFile( RAMDirectory* _directory )
  {
     length = 0;
	   lastModified = Misc::currentTimeMillis();
	   this->directory = _directory;
	   sizeInBytes = 0;
     #ifdef _DEBUG
     filename = NULL;
     #endif
  }

  RAMFile::~RAMFile(){
  }

  int64_t RAMFile::getLength()
  {
	  SCOPED_LOCK_MUTEX(THIS_LOCK);
	  return length;
  }

  void RAMFile::setLength( int64_t length )
  {
	  SCOPED_LOCK_MUTEX(THIS_LOCK);
	  this->length = length;
  }

  uint64_t RAMFile::getLastModified()
  {
	  SCOPED_LOCK_MUTEX(THIS_LOCK);
	  return lastModified;
  }

  void RAMFile::setLastModified( const uint64_t lastModified )
  {
	  SCOPED_LOCK_MUTEX(THIS_LOCK);
	  this->lastModified = lastModified;
  }

  uint8_t* RAMFile::addBuffer( const int32_t size )
  {
	  SCOPED_LOCK_MUTEX(THIS_LOCK);
	  uint8_t* buffer = newBuffer(size);
	  RAMFileBuffer* rfb = _CLNEW RAMFileBuffer(buffer, size);
	  if ( directory != NULL ) {
		  SCOPED_LOCK_MUTEX(directory->THIS_LOCK);
		  buffers.push_back( rfb );
		  directory->sizeInBytes += size;
	  } else {
		buffers.push_back(rfb);
	  }
	  return buffer;
  }

  uint8_t* RAMFile::getBuffer( const int32_t index )
  {
	  SCOPED_LOCK_MUTEX(THIS_LOCK);
	  return buffers[index]->_buffer;
  }

  int32_t RAMFile::numBuffers() const
  {
	  return buffers.size();
  }

  uint8_t* RAMFile::newBuffer( const int32_t size )
  {
	  return _CL_NEWARRAY( uint8_t, size );
  }

  int64_t RAMFile::getSizeInBytes() const
  {
	  if ( directory != NULL ) {
		  SCOPED_LOCK_MUTEX(directory->THIS_LOCK);
		  return sizeInBytes;
	  }
	  return 0;
  }


  bool RAMDirectory::list(vector<string>* names) const{
    SCOPED_LOCK_MUTEX(files_mutex);

    FileMap::const_iterator itr = files->begin();
    while (itr != files->end()){
        names->push_back( string(itr->first) );
        ++itr;
    }
    return true;
  }

  RAMDirectory::RAMDirectory():
   Directory(),files(_CLNEW FileMap(true,true))
  {
    this->sizeInBytes = 0;
	  setLockFactory( _CLNEW SingleInstanceLockFactory() );
  }

  RAMDirectory::~RAMDirectory(){
   //todo: should call close directory?
	  _CLDELETE( lockFactory );
	  _CLDELETE( files );
  }

  void RAMDirectory::_copyFromDir(Directory* dir, bool closeDir)
  {
  	vector<string> names;
    dir->list(&names);
    uint8_t buf[CL_NS(store)::BufferedIndexOutput<IndexOutput>::BUFFER_SIZE];

    for (size_t i=0;i<names.size();++i ){
      // make place on ram disk
      IndexOutput* os = createOutput(names[i].c_str());
      // read current file
      IndexInput* is = dir->openInput(names[i].c_str());
      // and copy to ram disk
      //todo: this could be a problem when copying from big indexes...
      int64_t len = is->length();
      int64_t readCount = 0;
      while (readCount < len) {
          int32_t toRead = (int32_t)(readCount + CL_NS(store)::BufferedIndexOutput<IndexOutput>::BUFFER_SIZE > len ? len - readCount : CL_NS(store)::BufferedIndexOutput<IndexOutput>::BUFFER_SIZE);
          is->readBytes(buf, toRead);
          os->writeBytes(buf, toRead);
          readCount += toRead;
      }

      // graceful cleanup
      is->close();
      _CLDELETE(is);
      os->close();
      _CLDELETE(os);
    }
    if (closeDir)
       dir->close();
  }
  RAMDirectory::RAMDirectory(Directory* dir):
   Directory(),files( _CLNEW FileMap(true,true) )
  {
    this->sizeInBytes = 0;
	  setLockFactory( _CLNEW SingleInstanceLockFactory() );
    _copyFromDir(dir,false);
  }

   RAMDirectory::RAMDirectory(const char* dir):
      Directory(),files( _CLNEW FileMap(true,true) )
   {
      this->sizeInBytes = 0;
      Directory* fsdir = FSDirectory::getDirectory(dir,false);
      try{
         _copyFromDir(fsdir,false);
      }_CLFINALLY(fsdir->close();_CLDECDELETE(fsdir););

   }

  bool RAMDirectory::fileExists(const char* name) const {
    SCOPED_LOCK_MUTEX(files_mutex);
    return files->exists((char*)name);
  }

  int64_t RAMDirectory::fileModified(const char* name) const {
	  SCOPED_LOCK_MUTEX(files_mutex);
	  RAMFile* f = files->get((char*)name);
	  return f->getLastModified();
  }

  int64_t RAMDirectory::fileLength(const char* name) const {
	  SCOPED_LOCK_MUTEX(files_mutex);
	  RAMFile* f = files->get((char*)name);
    return f->getLength();
  }


  bool RAMDirectory::openInput(const char* name, IndexInput*& ret, CLuceneError& error, int32_t /*bufferSize*/) {
    SCOPED_LOCK_MUTEX(files_mutex);
    RAMFile* file = files->get((char*)name);
    if (file == NULL) {
		  error.set(CL_ERR_IO, "[RAMDirectory::open] The requested file does not exist.");
		  return false;
    }
    ret = _CLNEW RAMInputStream<IndexInput>( file );
	  return true;
  }

  void RAMDirectory::close(){
      SCOPED_LOCK_MUTEX(files_mutex);
      files->clear();
      _CLDELETE(files);
  }

  bool RAMDirectory::doDeleteFile(const char* name) {
    SCOPED_LOCK_MUTEX(files_mutex);
    files->removeitr( files->find((char*)name) );
    return true;
  }

  void RAMDirectory::renameFile(const char* from, const char* to) {
	SCOPED_LOCK_MUTEX(files_mutex);
	FileMap::iterator itr = files->find((char*)from);

    /* DSR:CL_BUG_LEAK:
    ** If a file named $to already existed, its old value was leaked.
    ** My inclination would be to prevent this implicit deletion with an
    ** exception, but it happens routinely in CLucene's internals (e.g., during
    ** IndexWriter.addIndexes with the file named 'segments'). */
    if (files->exists((char*)to)) {
      files->removeitr( files->find((char*)to) );
    }
    if ( itr == files->end() ){
      char tmp[1024];
      _snprintf(tmp,1024,"cannot rename %s, file does not exist",from);
      _CLTHROWT(CL_ERR_IO,tmp);
    }
    CND_PRECONDITION(itr != files->end(), "itr==files->end()")
    RAMFile* file = itr->second;
    files->removeitr(itr,false,true);
    files->put(STRDUP_AtoA(to), file);
  }


  void RAMDirectory::touchFile(const char* name) {
    RAMFile* file = NULL;
    {
      SCOPED_LOCK_MUTEX(files_mutex);
      file = files->get((char*)name);
	}
    const uint64_t ts1 = file->getLastModified();
    uint64_t ts2 = Misc::currentTimeMillis();

	//make sure that the time has actually changed
    while ( ts1==ts2 ) {
        _LUCENE_SLEEP(1);
        ts2 = Misc::currentTimeMillis();
    };

    file->setLastModified(ts2);
  }

  IndexOutput* RAMDirectory::createOutput(const char* name) {
    /* Check the $files VoidMap to see if there was a previous file named
    ** $name.  If so, delete the old RAMFile object, but reuse the existing
    ** char buffer ($n) that holds the filename.  If not, duplicate the
    ** supplied filename buffer ($name) and pass ownership of that memory ($n)
    ** to $files. */

    SCOPED_LOCK_MUTEX(files_mutex);

	// get the actual pointer to the output name
    char* n = NULL;
	FileMap::const_iterator itr = files->find(const_cast<char*>(name));
	if ( itr!=files->end() )  {
		n = itr->first;
		RAMFile* rf = itr->second;
		_CLDELETE(rf);
	} else {
		n = STRDUP_AtoA(name);
	}

    RAMFile* file = _CLNEW RAMFile();
    #ifdef _DEBUG
      file->filename = n;
    #endif
    (*files)[n] = file;

    return _CLNEW RAMOutputStream<IndexOutput>(file);
  }

  std::string RAMDirectory::toString() const{
	  return "RAMDirectory";
  }

  const char* RAMDirectory::getClassName(){
	  return "RAMDirectory";
  }
  const char* RAMDirectory::getObjectName() const{
	  return getClassName();
  }

CL_NS_END
