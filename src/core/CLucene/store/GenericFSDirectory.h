/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_store_GenericFSDirectory_
#define _lucene_store_GenericFSDirectory_


#include "Directory.h"
#include "IndexInput.h"
#include "IndexOutput.h"
#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>

//#include "Lock.h"
//#include "LockFactory.h"
#include "CLucene/util/VoidMap.h"
CL_CLASS_DEF(util,StringBuffer)

   CL_NS_DEF(store)

   /**
   * Straightforward implementation of {@link lucene::store::Directory} as a directory of files.
   * <p>If the system property 'disableLuceneLocks' has the String value of
   * "true", lock creation will be disabled.
   *
   * @see Directory
   */
	template<
		typename index_input_base,
		typename index_output_base
	>
	class CLUCENE_EXPORT GenericFSDirectory:public Directory{
	private:
		class FSIndexOutput;
		class FSIndexInput;
		friend class GenericFSDirectory<
		      index_input_base,
		      index_output_base
		  >::FSIndexOutput;
		friend class GenericFSDirectory<
		      index_input_base,
		      index_output_base
		  >::FSIndexInput;

	protected:
		GenericFSDirectory(const char* path, const bool createDir, LockFactory* lockFactory=NULL);
	private:
    std::string directory;
		int refCount;
		void create();

		static const char* LOCK_DIR;
		static const char* getLockDir();
		char* getLockPrefix() const;
		static bool disableLocks;

		void priv_getFN(char* buffer, const char* name) const;
		bool useMMap;

	protected:
		/// Removes an existing file in the directory.
		bool doDeleteFile(const char* name);

	public:
	  ///Destructor - only call this if you are sure the directory
	  ///is not being used anymore. Otherwise use the ref-counting
	  ///facilities of _CLDECDELETE
		~GenericFSDirectory();

		/// Get a list of strings, one for each file in the directory.
		bool list(std::vector<std::string>* names) const;

		/// Returns true iff a file with the given name exists.
		bool fileExists(const char* name) const;

      /// Returns the text name of the directory
		const char* getDirName() const; ///<returns reference


    /**
    Returns the directory instance for the named location.

    Do not delete this instance, only use close, otherwise other instances
    will lose this instance.

    <p>Directories are cached, so that, for a given canonical path, the same
    GenericFSDirectory instance will always be returned.  This permits
    synchronization on directories.

    @param file the path to the directory.
    @param create if true, create, or erase any existing contents.
    @return the GenericFSDirectory for the named file.
    */
		static GenericFSDirectory<
		      index_input_base,
		      index_output_base
		  >* getDirectory(const char* file, const bool create=false, LockFactory* lockFactory=NULL);

		/// Returns the time the named file was last modified.
		int64_t fileModified(const char* name) const;

		//static
		/// Returns the time the named file was last modified.
		static int64_t fileModified(const char* dir, const char* name);

		//static
		/// Returns the length in bytes of a file in the directory.
		int64_t fileLength(const char* name) const;

		/// Returns a stream reading an existing file.
		bool openInput(const char* name, IndexInput*& ret, CLuceneError& err, int32_t bufferSize=-1);

		IndexInput* openMMapFile(const char* name, int32_t bufferSize=LUCENE_STREAM_BUFFER_SIZE);

		/// Renames an existing file in the directory.
		void renameFile(const char* from, const char* to);

      	/** Set the modified time of an existing file to now. */
      	void touchFile(const char* name);

		/// Creates a new, empty file in the directory with the given name.
		///	Returns a stream writing this file.
		IndexOutput* createOutput(const char* name);

		  ///Decrease the ref-count to the directory by one. If
		  ///the object is no longer needed, then the object is
		  ///removed from the directory pool.
      void close();

	  /**
	  * If MMap is available, this can disable use of
	  * mmap reading.
	  */
	  void setUseMMap(bool value);
	  /**
	  * Gets whether the directory is using MMap for inputstreams.
	  */
	  bool getUseMMap() const;

	  std::string toString() const;

		static const char* getClassName();
		const char* getObjectName() const;

	  /**
	  * Set whether Lucene's use of lock files is disabled. By default,
	  * lock files are enabled. They should only be disabled if the index
	  * is on a read-only medium like a CD-ROM.
	  */
	  static void setDisableLocks(bool doDisableLocks);

	  /**
	  * Returns whether Lucene's use of lock files is disabled.
	  * @return true if locks are disabled, false if locks are enabled.
	  */
	  static bool getDisableLocks();

	static CL_NS(util)::CLHashMap<const char*,GenericFSDirectory<
		index_input_base,
		index_output_base
	>*,CL_NS(util)::Compare::Char,CL_NS(util)::Equals::Char> DIRECTORIES;
	STATIC_DEFINE_MUTEX(DIRECTORIES_LOCK)

  };

CL_NS_END
#include "CLucene/_ApiHeader.h"

#include <fcntl.h>
#ifdef _CL_HAVE_IO_H
	#include <io.h>
#endif
#ifdef _CL_HAVE_SYS_STAT_H
	#include <sys/stat.h>
#endif
#ifdef _CL_HAVE_UNISTD_H
	#include <unistd.h>
#endif
#ifdef _CL_HAVE_DIRECT_H
	#include <direct.h>
#endif
#include <errno.h>

#include "LockFactory.h"
#include "CLucene/index/IndexReader.h"
#include "CLucene/index/IndexWriter.h"
#include "CLucene/util/Misc.h"
#include "CLucene/util/_MD5Digester.h"

#ifdef LUCENE_FS_MMAP
    #include "_MMap.h"
#endif

CL_NS_DEF(store)
CL_NS_USE(util)

   /** This cache of directories ensures that there is a unique Directory
   * instance per path, so that synchronization on the Directory can be used to
   * synchronize access between readers and writers.
   */
	template<
		typename index_input_base,
		typename index_output_base
	>
	CL_NS(util)::CLHashMap<const char*,GenericFSDirectory<
		index_input_base,
		index_output_base
	>*,CL_NS(util)::Compare::Char,CL_NS(util)::Equals::Char> GenericFSDirectory<
                index_input_base,
                index_output_base
        >::DIRECTORIES(false,false);

#ifndef _CL_DISABLE_MULTITHREADING
	template<
		typename index_input_base,
		typename index_output_base
	>
	_LUCENE_THREADMUTEX GenericFSDirectory<
		index_input_base,
		index_output_base
	>::DIRECTORIES_LOCK;
#endif

	template<
		typename index_input_base,
		typename index_output_base
	>
	bool GenericFSDirectory<
		index_input_base,
		index_output_base
	>::disableLocks=false;


	template<
		typename index_input_base,
		typename index_output_base
	>
	class GenericFSDirectory<
		index_input_base,
		index_output_base
	>::FSIndexInput:public index_input_base {
		/**
		* We used a shared handle between all the fsindexinput clones.
		* This reduces number of file handles we need, and it means
		* we dont have to use file tell (which is slow) before doing
		* a read.
    * TODO: get rid of this and dup/fctnl or something like that...
		*/
		class SharedHandle {
		public:
			int32_t fhandle;
			int64_t _length;
			int64_t _fpos;
			DEFINE_MUTEX(*THIS_LOCK)
			char path[CL_MAX_DIR]; //todo: this is only used for cloning, better to get information from the fhandle
			SharedHandle(const char* path);
			~SharedHandle();
		};
		boost::shared_ptr<SharedHandle> handle;
		int64_t _pos;
		FSIndexInput(boost::shared_ptr<SharedHandle> const& handle, int32_t __bufferSize):
			index_input_base(__bufferSize)
		{
			this->_pos = 0;
			this->handle = handle;
		};
	protected:
		FSIndexInput(const FSIndexInput& clone);
	public:
		static bool open(const char* path, IndexInput*& ret, CLuceneError& error, int32_t bufferSize=-1);
		~FSIndexInput();

		IndexInput* clone() const;
		void close();
		int64_t length() const { return handle.get()->_length; }

		const char* getDirectoryType() const{ return GenericFSDirectory<
		        index_input_base,
		        index_output_base
		    >::getClassName(); }
    const char* getObjectName() const{ return getClassName(); }
    static const char* getClassName() { return "FSIndexInput"; }
	protected:
		// Random-access methods
		void seekInternal(const int64_t position);
		// IndexInput methods
		void readInternal(uint8_t* b, const int32_t len);
	};

	template<
		typename index_input_base,
		typename index_output_base
	>
	class GenericFSDirectory<
		index_input_base,
		index_output_base
	>::FSIndexOutput: public index_output_base {
	private:
		int32_t fhandle;
	protected:
		// output methods:
		void flushBuffer(const uint8_t* b, const int32_t size);
	public:
		FSIndexOutput(const char* path);
		~FSIndexOutput();

		// output methods:
		void close();

		// Random-access methods
		void seek(const int64_t pos);
		int64_t length() const;
	};

	template<
		typename index_input_base,
		typename index_output_base
	>
	bool GenericFSDirectory<
		index_input_base,
		index_output_base
	>::FSIndexInput::open(const char* path, IndexInput*& ret, CLuceneError& error, int32_t __bufferSize )    {
	//Func - Constructor.
	//       Opens the file named path
	//Pre  - path != NULL
	//Post - if the file could not be opened  an exception is thrown.

	  CND_PRECONDITION(path != NULL, "path is NULL");

	  if ( __bufferSize == -1 )
		  __bufferSize = index_output_base::BUFFER_SIZE;
	  boost::shared_ptr<SharedHandle> handle(_CLNEW SharedHandle(path));

	  //Open the file
	  handle.get()->fhandle  = ::_cl_open(path, _O_BINARY | O_RDONLY | _O_RANDOM, _S_IREAD );

	  //Check if a valid handle was retrieved
	  if (handle.get()->fhandle >= 0){
		  //Store the file length
		  handle.get()->_length = fileSize(handle.get()->fhandle);
		  if ( handle.get()->_length == -1 )
	  		error.set( CL_ERR_IO,"fileStat error" );
		  else{
			  handle.get()->_fpos = 0;
			  ret = _CLNEW FSIndexInput(handle, __bufferSize);
			  return true;
		  }
	  }else{
		  int err = errno;
      if ( err == ENOENT )
	      error.set(CL_ERR_IO, "File does not exist");
      else if ( err == EACCES )
        error.set(CL_ERR_IO, "File Access denied");
      else if ( err == EMFILE )
        error.set(CL_ERR_IO, "Too many open files");
      else
      	error.set(CL_ERR_IO, "Could not open file");
	  }
#ifndef _CL_DISABLE_MULTITHREADING
    delete handle.get()->THIS_LOCK;
#endif
	  return false;
  }

	template<
		typename index_input_base,
		typename index_output_base
	>
  GenericFSDirectory<
		index_input_base,
		index_output_base
	>::FSIndexInput::FSIndexInput(const FSIndexInput& other): index_input_base(other){
  //Func - Constructor
  //       Uses clone for its initialization
  //Pre  - clone is a valide instance of FSIndexInput
  //Post - The instance has been created and initialized by clone
	  if ( other.handle.get() == NULL )
		  _CLTHROWA(CL_ERR_NullPointer, "other handle is null");

	  SCOPED_LOCK_MUTEX(*other.handle.get()->THIS_LOCK)
	  handle = other.handle;
	  _pos = other.handle.get()->_fpos; //note where we are currently...
  }

	template<
		typename index_input_base,
		typename index_output_base
	>
  GenericFSDirectory<
		index_input_base,
		index_output_base
	>::FSIndexInput::SharedHandle::SharedHandle(const char* path){
  	fhandle = 0;
    _length = 0;
    _fpos = 0;
    strcpy(this->path,path);

#ifndef _CL_DISABLE_MULTITHREADING
	  THIS_LOCK = new _LUCENE_THREADMUTEX;
#endif
  }
	template<
		typename index_input_base,
		typename index_output_base
	>
  GenericFSDirectory<
		index_input_base,
		index_output_base
	>::FSIndexInput::SharedHandle::~SharedHandle() {
    if ( fhandle >= 0 ){
      if ( ::_close(fhandle) != 0 )
        _CLTHROWA(CL_ERR_IO, "File IO Close error");
      else
        fhandle = -1;
    }
  }

	template<
		typename index_input_base,
		typename index_output_base
	>
  GenericFSDirectory<
		index_input_base,
		index_output_base
	>::FSIndexInput::~FSIndexInput(){
  //Func - Destructor
  //Pre  - True
  //Post - The file for which this instance is responsible has been closed.
  //       The instance has been destroyed

	  FSIndexInput::close();
  }

	template<
		typename index_input_base,
		typename index_output_base
	>
  IndexInput* GenericFSDirectory<
		index_input_base,
		index_output_base
	>::FSIndexInput::clone() const
  {
    return _CLNEW typename GenericFSDirectory<
		index_input_base,
		index_output_base
	>::FSIndexInput(*this);
  }
	template<
		typename index_input_base,
		typename index_output_base
	>
  void GenericFSDirectory<
		index_input_base,
		index_output_base
	>::FSIndexInput::close()  {
	index_input_base::close();
#ifndef _CL_DISABLE_MULTITHREADING
	if ( handle.get() != NULL ){
		boost::shared_ptr<SharedHandle> for_locking = handle;
		SCOPED_LOCK_MUTEX(*for_locking.get()->THIS_LOCK)
		handle.reset();
	}
#else
	handle.reset();
#endif
  }

	template<
		typename index_input_base,
		typename index_output_base
	>
  void GenericFSDirectory<
		index_input_base,
		index_output_base
	>::FSIndexInput::seekInternal(const int64_t position)  {
	CND_PRECONDITION(position>=0 &&position<handle.get()->_length,"Seeking out of range")
	_pos = position;
  }

/** IndexInput methods */
	template<
		typename index_input_base,
		typename index_output_base
	>
void GenericFSDirectory<
		index_input_base,
		index_output_base
	>::FSIndexInput::readInternal(uint8_t* b, const int32_t len) {
	CND_PRECONDITION(handle.get()!=NULL,"shared file handle has closed");
	CND_PRECONDITION(handle.get()->fhandle>=0,"file is not open");
	SCOPED_LOCK_MUTEX(*handle.get()->THIS_LOCK)

	if ( handle.get()->_fpos != _pos ){
		if ( fileSeek(handle.get()->fhandle,_pos,SEEK_SET) != _pos ){
			_CLTHROWA( CL_ERR_IO, "File IO Seek error");
		}
		handle.get()->_fpos = _pos;
	}

	this->bufferLength = _read(handle.get()->fhandle,b,len); // 2004.10.31:SF 1037836
	if (this->bufferLength == 0){
		_CLTHROWA(CL_ERR_IO, "read past EOF");
	}
	if (this->bufferLength == -1){
		//if (EINTR == errno) we could do something else... but we have
		//to guarantee some return, or throw EOF

		_CLTHROWA(CL_ERR_IO, "read error");
	}
	_pos+=this->bufferLength;
	handle.get()->_fpos=_pos;
}

	template<
		typename index_input_base,
		typename index_output_base
	>
  GenericFSDirectory<
		index_input_base,
		index_output_base
	>::FSIndexOutput::FSIndexOutput(const char* path){
	//O_BINARY - Opens file in binary (untranslated) mode
	//O_CREAT - Creates and opens new file for writing. Has no effect if file specified by filename exists
	//O_RANDOM - Specifies that caching is optimized for, but not restricted to, random access from disk.
	//O_WRONLY - Opens file for writing only;
	  if ( Misc::dir_Exists(path) )
	    fhandle = _cl_open( path, _O_BINARY | O_RDWR | _O_RANDOM | O_TRUNC, _S_IREAD | _S_IWRITE);
	  else // added by JBP
	    fhandle = _cl_open( path, _O_BINARY | O_RDWR | _O_RANDOM | O_CREAT, _S_IREAD | _S_IWRITE);

	  if ( fhandle < 0 ){
      int err = errno;
      if ( err == ENOENT )
	      _CLTHROWA(CL_ERR_IO, "File does not exist");
      else if ( err == EACCES )
          _CLTHROWA(CL_ERR_IO, "File Access denied");
      else if ( err == EMFILE )
          _CLTHROWA(CL_ERR_IO, "Too many open files");
    }
  }
	template<
		typename index_input_base,
		typename index_output_base
	>
  GenericFSDirectory<
		index_input_base,
		index_output_base
	>::FSIndexOutput::~FSIndexOutput(){
	if ( fhandle >= 0 ){
	  try {
        FSIndexOutput::close();
	  }catch(CLuceneError& err){
	    //ignore IO errors...
	    if ( err.number() != CL_ERR_IO )
	        throw;
	  }
	}
  }

  /** output methods: */
	template<
		typename index_input_base,
		typename index_output_base
	>
  void GenericFSDirectory<
		index_input_base,
		index_output_base
	>::FSIndexOutput::flushBuffer(const uint8_t* b, const int32_t size) {
	  CND_PRECONDITION(fhandle>=0,"file is not open");
      if ( size > 0 && _write(fhandle,b,size) != size )
        _CLTHROWA(CL_ERR_IO, "File IO Write error");
  }
	template<
		typename index_input_base,
		typename index_output_base
	>
  void GenericFSDirectory<
		index_input_base,
		index_output_base
	>::FSIndexOutput::close() {
    try{
      index_output_base::close();
    }catch(CLuceneError& err){
	    //ignore IO errors...
	    if ( err.number() != CL_ERR_IO )
	        throw;
    }

    if ( ::_close(fhandle) != 0 )
      _CLTHROWA(CL_ERR_IO, "File IO Close error");
    else
      fhandle = -1; //-1 now indicates closed
  }

	template<
		typename index_input_base,
		typename index_output_base
	>
  void GenericFSDirectory<
		index_input_base,
		index_output_base
	>::FSIndexOutput::seek(const int64_t pos) {
    CND_PRECONDITION(fhandle>=0,"file is not open");
    index_output_base::seek(pos);
	int64_t ret = fileSeek(fhandle,pos,SEEK_SET);
	if ( ret != pos ){
      _CLTHROWA(CL_ERR_IO, "File IO Seek error");
	}
  }
	template<
		typename index_input_base,
		typename index_output_base
	>
  int64_t GenericFSDirectory<
		index_input_base,
		index_output_base
	>::FSIndexOutput::length() const {
	  CND_PRECONDITION(fhandle>=0,"file is not open");
	  return fileSize(fhandle);
  }


	template<
		typename index_input_base,
		typename index_output_base
	>
	const char* GenericFSDirectory<
		index_input_base,
		index_output_base
	>::LOCK_DIR=NULL;
	template<
		typename index_input_base,
		typename index_output_base
	>
	const char* GenericFSDirectory<
		index_input_base,
		index_output_base
	>::getLockDir(){
		#ifdef LUCENE_LOCK_DIR
		LOCK_DIR = LUCENE_LOCK_DIR;
		#else
			#ifdef LUCENE_LOCK_DIR_ENV_1
			if ( LOCK_DIR == NULL )
				LOCK_DIR = getenv(LUCENE_LOCK_DIR_ENV_1);
			#endif
			#ifdef LUCENE_LOCK_DIR_ENV_2
			if ( LOCK_DIR == NULL )
				LOCK_DIR = getenv(LUCENE_LOCK_DIR_ENV_2);
			#endif
			#ifdef LUCENE_LOCK_DIR_ENV_FALLBACK
			if ( LOCK_DIR == NULL )
				LOCK_DIR=LUCENE_LOCK_DIR_ENV_FALLBACK;
			#endif
			if ( LOCK_DIR == NULL )
				_CLTHROWA(CL_ERR_IO, "Couldn't get determine lock dir");
		#endif

		return LOCK_DIR;
	}

	template<
		typename index_input_base,
		typename index_output_base
	>
  GenericFSDirectory<
		index_input_base,
		index_output_base
	>::GenericFSDirectory(const char* _path, const bool createDir, LockFactory* lockFactory):
   Directory(),
   refCount(0),
   useMMap(LUCENE_USE_MMAP)
  {
    directory = _path;
    bool doClearLockID = false;

    if ( lockFactory == NULL ) {
    	if ( disableLocks ) {
    		lockFactory = NoLockFactory::getNoLockFactory();
    	} else {
    		lockFactory = _CLNEW FSLockFactory( directory.c_str() );
    		doClearLockID = true;
    	}
    }

    setLockFactory( lockFactory );

    if ( doClearLockID ) {
    	lockFactory->setLockPrefix(NULL);
    }

    if (createDir) {
      create();
    }

    if (!Misc::dir_Exists(directory.c_str())){
      char* err = _CL_NEWARRAY(char,19+directory.length()+1); //19: len of " is not a directory"
      strcpy(err,directory.c_str());
      strcat(err," is not a directory");
      _CLTHROWA_DEL(CL_ERR_IO, err );
    }

  }


	template<
		typename index_input_base,
		typename index_output_base
	>
  void GenericFSDirectory<
		index_input_base,
		index_output_base
	>::create(){
    SCOPED_LOCK_MUTEX(THIS_LOCK)
    struct cl_stat_t fstat;
    if ( fileStat(directory.c_str(),&fstat) != 0 ) {
	  	//todo: should construct directory using _mkdirs... have to write replacement
      if ( _mkdir(directory.c_str()) == -1 ){
			  char* err = _CL_NEWARRAY(char,27+directory.length()+1); //27: len of "Couldn't create directory: "
			  strcpy(err,"Couldn't create directory: ");
			  strcat(err,directory.c_str());
			  _CLTHROWA_DEL(CL_ERR_IO, err );
      }
		}

		if ( fileStat(directory.c_str(),&fstat) != 0 || !(fstat.st_mode & S_IFDIR) ){
	      char tmp[1024];
	      _snprintf(tmp,1024,"%s not a directory", directory.c_str());
	      _CLTHROWA(CL_ERR_IO,tmp);
		}

	  //clear old files
	  vector<string> files;
	  Misc::listFiles(directory.c_str(), files, false);
	  vector<string>::iterator itr = files.begin();
	  while ( itr != files.end() ){
	  	if ( CL_NS(index)::IndexReader::isLuceneFile(itr->c_str()) ){
        if ( _unlink( (directory + PATH_DELIMITERA + *itr).c_str() ) == -1 ) {
				  _CLTHROWA(CL_ERR_IO, "Couldn't delete file "); //todo: make richer error
				}
	  	}
	  	itr++;
	  }
    lockFactory->clearLock( CL_NS(index)::IndexWriter::WRITE_LOCK_NAME );

  }

	template<
		typename index_input_base,
		typename index_output_base
	>
  void GenericFSDirectory<
		index_input_base,
		index_output_base
	>::priv_getFN(char* buffer, const char* name) const{
      buffer[0] = 0;
      strcpy(buffer,directory.c_str());
      strcat(buffer, PATH_DELIMITERA );
      strcat(buffer,name);
  }

	template<
		typename index_input_base,
		typename index_output_base
	>
  GenericFSDirectory<
		index_input_base,
		index_output_base
	>::~GenericFSDirectory(){
	  _CLDELETE( lockFactory );
  }


	template<
		typename index_input_base,
		typename index_output_base
	>
    void GenericFSDirectory<
		index_input_base,
		index_output_base
	>::setUseMMap(bool value){ useMMap = value; }
	template<
		typename index_input_base,
		typename index_output_base
	>
    bool GenericFSDirectory<
		index_input_base,
		index_output_base
	>::getUseMMap() const{ return useMMap; }
	template<
		typename index_input_base,
		typename index_output_base
	>
    const char* GenericFSDirectory<
		index_input_base,
		index_output_base
	>::getClassName(){
      return "FSDirectory";
    }
	template<
		typename index_input_base,
		typename index_output_base
	>
    const char* GenericFSDirectory<
		index_input_base,
		index_output_base
	>::getObjectName() const{
      return getClassName();
    }

	template<
		typename index_input_base,
		typename index_output_base
	>
    void GenericFSDirectory<
		index_input_base,
		index_output_base
	>::setDisableLocks(bool doDisableLocks) { disableLocks = doDisableLocks; }
	template<
		typename index_input_base,
		typename index_output_base
	>
    bool GenericFSDirectory<
		index_input_base,
		index_output_base
	>::getDisableLocks() { return disableLocks; }


	template<
		typename index_input_base,
		typename index_output_base
	>
  bool GenericFSDirectory<
		index_input_base,
		index_output_base
	>::list(vector<string>* names) const{ //todo: fix this, ugly!!!
    CND_PRECONDITION(!directory.empty(),"directory is not open");
    return Misc::listFiles(directory.c_str(), *names, false);
  }

	template<
		typename index_input_base,
		typename index_output_base
	>
  bool GenericFSDirectory<
		index_input_base,
		index_output_base
	>::fileExists(const char* name) const {
	  CND_PRECONDITION(directory[0]!=0,"directory is not open");
    char fl[CL_MAX_DIR];
    priv_getFN(fl, name);
    return Misc::dir_Exists( fl );
  }

	template<
		typename index_input_base,
		typename index_output_base
	>
  const char* GenericFSDirectory<
		index_input_base,
		index_output_base
	>::getDirName() const{
    return directory.c_str();
  }

  //static
	template<
		typename index_input_base,
		typename index_output_base
	>
  GenericFSDirectory<
		index_input_base,
		index_output_base
	>* GenericFSDirectory<
		index_input_base,
		index_output_base
	>::getDirectory(const char* file, const bool _create, LockFactory* lockFactory){
    GenericFSDirectory<
		index_input_base,
		index_output_base
	>* dir = NULL;
	{
		if ( !file || !*file )
			_CLTHROWA(CL_ERR_IO,"Invalid directory");

    char buf[CL_MAX_PATH];
  	char* tmpdirectory = _realpath(file,buf);//set a realpath so that if we change directory, we can still function
  	if ( !tmpdirectory || !*tmpdirectory ){
  		strncpy(buf,file, CL_MAX_PATH);
      tmpdirectory = buf;
  	}

		SCOPED_LOCK_MUTEX(DIRECTORIES_LOCK)
		dir = DIRECTORIES.get(tmpdirectory);
		if ( dir == NULL  ){
			dir = _CLNEW GenericFSDirectory<
		index_input_base,
		index_output_base
	>(tmpdirectory,_create,lockFactory);
			DIRECTORIES.put( dir->directory.c_str(), dir);
		} else if ( _create ) {
	    	dir->create();
		} else {
			if ( lockFactory != NULL && lockFactory != dir->getLockFactory() ) {
				_CLTHROWA(CL_ERR_IO,"Directory was previously created with a different LockFactory instance, please pass NULL as the lockFactory instance and use setLockFactory to change it");
			}
		}

		{
			SCOPED_LOCK_MUTEX(dir->THIS_LOCK)
				dir->refCount++;
		}
	}

    return _CL_POINTER(dir);
  }

	template<
		typename index_input_base,
		typename index_output_base
	>
  int64_t GenericFSDirectory<
		index_input_base,
		index_output_base
	>::fileModified(const char* name) const {
	CND_PRECONDITION(directory[0]!=0,"directory is not open");
    struct cl_stat_t buf;
    char buffer[CL_MAX_DIR];
    priv_getFN(buffer,name);
    if (fileStat( buffer, &buf ) == -1 )
      return 0;
    else
      return buf.st_mtime;
  }

  //static
	template<
		typename index_input_base,
		typename index_output_base
	>
  int64_t GenericFSDirectory<
		index_input_base,
		index_output_base
	>::fileModified(const char* dir, const char* name){
    struct cl_stat_t buf;
    char buffer[CL_MAX_DIR];
	_snprintf(buffer,CL_MAX_DIR,"%s%s%s",dir,PATH_DELIMITERA,name);
    fileStat( buffer, &buf );
    return buf.st_mtime;
  }

	template<
		typename index_input_base,
		typename index_output_base
	>
  void GenericFSDirectory<
		index_input_base,
		index_output_base
	>::touchFile(const char* name){
	  CND_PRECONDITION(directory[0]!=0,"directory is not open");
    char buffer[CL_MAX_DIR];
    _snprintf(buffer,CL_MAX_DIR,"%s%s%s",directory.c_str(),PATH_DELIMITERA,name);

    int32_t r = _cl_open(buffer, O_RDWR, _S_IWRITE);
	if ( r < 0 )
		_CLTHROWA(CL_ERR_IO,"IO Error while touching file");
	::_close(r);
  }

	template<
		typename index_input_base,
		typename index_output_base
	>
  int64_t GenericFSDirectory<
		index_input_base,
		index_output_base
	>::fileLength(const char* name) const {
	  CND_PRECONDITION(directory[0]!=0,"directory is not open");
    struct cl_stat_t buf;
    char buffer[CL_MAX_DIR];
    priv_getFN(buffer,name);
    if ( fileStat( buffer, &buf ) == -1 )
      return 0;
    else
      return buf.st_size;
  }

	template<
		typename index_input_base,
		typename index_output_base
	>
  IndexInput* GenericFSDirectory<
		index_input_base,
		index_output_base
	>::openMMapFile(const char* name, int32_t bufferSize){
#ifdef LUCENE_FS_MMAP
    char fl[CL_MAX_DIR];
    priv_getFN(fl, name);
	if ( Misc::file_Size(fl) < LUCENE_INT32_MAX_SHOULDBE ) //todo: would this be bigger on 64bit systems?. i suppose it would be...test first
		return _CLNEW MMapIndexInput( fl );
	else
		return _CLNEW FSIndexInput( fl, bufferSize );
#else
	_CLTHROWA(CL_ERR_Runtime,"MMap not enabled at compilation");
#endif
  }

	template<
		typename index_input_base,
		typename index_output_base
	>
  bool GenericFSDirectory<
		index_input_base,
		index_output_base
	>::openInput(const char * name, IndexInput *& ret, CLuceneError& error, int32_t bufferSize)
  {
	CND_PRECONDITION(directory[0]!=0,"directory is not open")
    char fl[CL_MAX_DIR];
    priv_getFN(fl, name);
#ifdef LUCENE_FS_MMAP
	//todo: do some tests here... like if the file
	//is >2gb, then some system cannot mmap the file
	//also some file systems mmap will fail?? could detect here too
	if ( useMMap && Misc::file_Size(fl) < LUCENE_INT32_MAX_SHOULDBE ) //todo: would this be bigger on 64bit systems?. i suppose it would be...test first
		return MMapIndexInput( fl, ret, error, bufferSize );
	else
#endif
	return FSIndexInput::open( fl, ret, error, bufferSize );
  }

	template<
		typename index_input_base,
		typename index_output_base
	>
  void GenericFSDirectory<
		index_input_base,
		index_output_base
	>::close(){
    SCOPED_LOCK_MUTEX(DIRECTORIES_LOCK)
    {
	    SCOPED_LOCK_MUTEX(THIS_LOCK)

	    CND_PRECONDITION(directory[0]!=0,"directory is not open");

	    if (--refCount <= 0 ) {//refcount starts at 1
	        Directory* dir = DIRECTORIES.get(getDirName());
	        if(dir){
	            DIRECTORIES.remove( getDirName() ); //this will be removed in ~GenericFSDirectory
	            _CLDECDELETE(dir);
	        }
	    }
	}
   }

   /**
   * So we can do some byte-to-hexchar conversion below
   */
	const char HEX_DIGITS[] =
	{'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};

	template<
		typename index_input_base,
		typename index_output_base
	>
	char* GenericFSDirectory<
		index_input_base,
		index_output_base
	>::getLockPrefix() const{
		char dirName[CL_MAX_PATH]; // name to be hashed
		if ( _realpath(directory.c_str(),dirName) == NULL ){
			_CLTHROWA(CL_ERR_Runtime,"Invalid directory path");
		}

		//to make a compatible name with jlucene, we need to make some changes...
		if ( dirName[1] == ':' )
			dirName[0] = (char)_totupper((char)dirName[0]);

		char* smd5 = MD5String(dirName);

		char* ret=_CL_NEWARRAY(char,32+7+1); //32=2*16, 7=strlen("lucene-")
		strcpy(ret,"lucene-");
		strcat(ret,smd5);

		_CLDELETE_CaARRAY(smd5);

	    return ret;
  }

	template<
		typename index_input_base,
		typename index_output_base
	>
  bool GenericFSDirectory<
		index_input_base,
		index_output_base
	>::doDeleteFile(const char* name)  {
	CND_PRECONDITION(directory[0]!=0,"directory is not open");
    char fl[CL_MAX_DIR];
    priv_getFN(fl, name);
	return _unlink(fl) != -1;
  }

	template<
		typename index_input_base,
		typename index_output_base
	>
  void GenericFSDirectory<
		index_input_base,
		index_output_base
	>::renameFile(const char* from, const char* to){
	CND_PRECONDITION(directory[0]!=0,"directory is not open");
    SCOPED_LOCK_MUTEX(THIS_LOCK)
    char old[CL_MAX_DIR];
    priv_getFN(old, from);

    char nu[CL_MAX_DIR];
    priv_getFN(nu, to);

    /* This is not atomic.  If the program crashes between the call to
    delete() and the call to renameTo() then we're screwed, but I've
    been unable to figure out how else to do this... */

    if ( Misc::dir_Exists(nu) ){
      //we run this sequence of unlinking an arbitary 100 times
      //on some platforms (namely windows), there can be a
      //delay between unlink and dir_exists==false
      while ( true ){
          if( _unlink(nu) != 0 ){
    	    char* err = _CL_NEWARRAY(char,16+strlen(to)+1); //16: len of "couldn't delete "
    		strcpy(err,"couldn't delete ");
    		strcat(err,to);
            _CLTHROWA_DEL(CL_ERR_IO, err );
          }
          //we can wait until the dir_Exists() returns false
          //after the success run of unlink()
          int i=0;
		  while ( Misc::dir_Exists(nu) && i < 100 ){
			  if ( ++i > 50 ) //if it still doesn't show up, then we do some sleeping for the last 50ms
				  _LUCENE_SLEEP(1);
		  }
          if ( !Misc::dir_Exists(nu) )
            break; //keep trying to unlink until the file is gone, or the unlink fails.
      }
    }
    if ( _rename(old,nu) != 0 ){
       //todo: jlucene has some extra rename code - if the rename fails, it copies
       //the whole file to the new file... might want to implement that if renaming
       //fails on some platforms
        char buffer[20+CL_MAX_PATH+CL_MAX_PATH];
        strcpy(buffer,"couldn't rename ");
        strcat(buffer,from);
        strcat(buffer," to ");
        strcat(buffer,nu);
      _CLTHROWA(CL_ERR_IO, buffer );
    }
  }

	template<
		typename index_input_base,
		typename index_output_base
	>
  IndexOutput* GenericFSDirectory<
		index_input_base,
		index_output_base
	>::createOutput(const char* name) {
	CND_PRECONDITION(directory[0]!=0,"directory is not open");
    char fl[CL_MAX_DIR];
    priv_getFN(fl, name);
	  if ( Misc::dir_Exists(fl) ){
		  if ( _unlink(fl) != 0 ){
			  char tmp[1024];
			  strcpy(tmp, "Cannot overwrite: ");
			  strcat(tmp, name);
			  _CLTHROWA(CL_ERR_IO, tmp);
		  }
	  }
    return _CLNEW FSIndexOutput( fl );
  }

	template<
		typename index_input_base,
		typename index_output_base
	>
  string GenericFSDirectory<
		index_input_base,
		index_output_base
	>::toString() const{
	  return string("FSDirectory@") + this->directory;
  }

CL_NS_END

#endif
