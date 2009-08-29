/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
*
* Distributable under the terms of either the Apache License (Version 2.0) or
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
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

#include "FSDirectory.h"
#include "_MMapDirectory.h"
#include "LockFactory.h"
#include "CLucene/index/IndexReader.h"
#include "CLucene/index/IndexWriter.h"
#include "CLucene/util/Misc.h"
#include "CLucene/util/_MD5Digester.h"

CL_NS_DEF(store)
CL_NS_USE(util)

   /** This cache of directories ensures that there is a unique Directory
   * instance per path, so that synchronization on the Directory can be used to
   * synchronize access between readers and writers.
   */
	static CL_NS(util)::CLHashMap<const char*,FSDirectory*,CL_NS(util)::Compare::Char,CL_NS(util)::Equals::Char> DIRECTORIES(false,false);
	STATIC_DEFINE_MUTEX(DIRECTORIES_LOCK)

	FSIOFactory FSDirectory::defaultIOFactory;
  bool FSDirectory::useMMap = LUCENE_USE_MMAP;
	bool FSDirectory::disableLocks=false;

	const char* FSDirectory::LOCK_DIR=NULL;
	const char* FSDirectory::getLockDir(){
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

  FSDirectory::FSDirectory(const char* _path, const bool createDir, LockFactory* lockFactory, IOFactory* ioFactory):
   Directory(),
   refCount(0),
   ioFactory(ioFactory)
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


  void FSDirectory::create(){
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

  void FSDirectory::priv_getFN(char* buffer, const char* name) const{
      buffer[0] = 0;
      strcpy(buffer,directory.c_str());
      strcat(buffer, PATH_DELIMITERA );
      strcat(buffer,name);
  }

  FSDirectory::~FSDirectory(){
	  _CLDELETE( lockFactory );
  }


  void FSDirectory::setUseMMap(bool value){ useMMap = value; }
  bool FSDirectory::getUseMMap() { return useMMap; }

  const char* FSDirectory::getClassName(){
    return "FSDirectory";
  }
  const char* FSDirectory::getObjectName() const{
    return getClassName();
  }

  void FSDirectory::setDisableLocks(bool doDisableLocks) { disableLocks = doDisableLocks; }
  bool FSDirectory::getDisableLocks() { return disableLocks; }


  bool FSDirectory::list(vector<string>* names) const{ //todo: fix this, ugly!!!
    CND_PRECONDITION(!directory.empty(),"directory is not open");
    return Misc::listFiles(directory.c_str(), *names, false);
  }

  bool FSDirectory::fileExists(const char* name) const {
	  CND_PRECONDITION(directory[0]!=0,"directory is not open");
    char fl[CL_MAX_DIR];
    priv_getFN(fl, name);
    return Misc::dir_Exists( fl );
  }

  const char* FSDirectory::getDirName() const{
    return directory.c_str();
  }

  //static
  FSDirectory* FSDirectory::getDirectory(const char* file, const bool _create, LockFactory* lockFactory, IOFactory* ioFactory){
    FSDirectory* dir = NULL;
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
      if ( getUseMMap() ){
        dir = _CLNEW MMapDirectory(tmpdirectory,_create,lockFactory);
      }else{
			  dir = _CLNEW FSDirectory(tmpdirectory,_create,lockFactory,ioFactory);
      }
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

  int64_t FSDirectory::fileModified(const char* name) const {
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
  int64_t FSDirectory::fileModified(const char* dir, const char* name){
    struct cl_stat_t buf;
    char buffer[CL_MAX_DIR];
	_snprintf(buffer,CL_MAX_DIR,"%s%s%s",dir,PATH_DELIMITERA,name);
    fileStat( buffer, &buf );
    return buf.st_mtime;
  }

  void FSDirectory::touchFile(const char* name){
	  CND_PRECONDITION(directory[0]!=0,"directory is not open");
    char buffer[CL_MAX_DIR];
    _snprintf(buffer,CL_MAX_DIR,"%s%s%s",directory.c_str(),PATH_DELIMITERA,name);

    int32_t r = _cl_open(buffer, O_RDWR, _S_IWRITE);
	if ( r < 0 )
		_CLTHROWA(CL_ERR_IO,"IO Error while touching file");
	::_close(r);
  }

  int64_t FSDirectory::fileLength(const char* name) const {
	  CND_PRECONDITION(directory[0]!=0,"directory is not open");
    struct cl_stat_t buf;
    char buffer[CL_MAX_DIR];
    priv_getFN(buffer,name);
    if ( fileStat( buffer, &buf ) == -1 )
      return 0;
    else
      return buf.st_size;
  }

  bool FSDirectory::openInput(const char * name, IndexInput *& ret, CLuceneError& error, int32_t bufferSize)
  {
	  CND_PRECONDITION(directory[0]!=0,"directory is not open")
    char fl[CL_MAX_DIR];
    priv_getFN(fl, name);
	return ioFactory->openInput( fl, ret, error, bufferSize );
  }

  void FSDirectory::close(){
    SCOPED_LOCK_MUTEX(DIRECTORIES_LOCK)
    {
	    SCOPED_LOCK_MUTEX(THIS_LOCK)

	    CND_PRECONDITION(directory[0]!=0,"directory is not open");

	    if (--refCount <= 0 ) {//refcount starts at 1
	        Directory* dir = DIRECTORIES.get(getDirName());
	        if(dir){
	            DIRECTORIES.remove( getDirName() ); //this will be removed in ~FSDirectory
	            _CLDECDELETE(dir);
	        }
	    }
	}
   }

   /**
   * So we can do some byte-to-hexchar conversion below
   */
	char HEX_DIGITS[] =
	{'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};

	char* FSDirectory::getLockPrefix() const{
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

  bool FSDirectory::doDeleteFile(const char* name)  {
	CND_PRECONDITION(directory[0]!=0,"directory is not open");
    char fl[CL_MAX_DIR];
    priv_getFN(fl, name);
	return _unlink(fl) != -1;
  }

  void FSDirectory::renameFile(const char* from, const char* to){
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

  IndexOutput* FSDirectory::createOutput(const char* name) {
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
    return ioFactory->newOutput( fl );
  }

  string FSDirectory::toString() const{
	  return string("FSDirectory@") + this->directory;
  }

CL_NS_END
