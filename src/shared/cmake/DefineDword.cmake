#defines a dword replacement

MACRO ( DEFINE_DWORD ) 

    IF ( HAVE_WINDOWS_H )
    	SET (CMAKE_EXTRA_INCLUDE_FILES "${CMAKE_EXTRA_INCLUDE_FILES};windows.h")
    ENDIF ( HAVE_WINDOWS_H )

    CHECK_TYPE_SIZE ( DWORD _CL_HAVE_TYPE_DWORD )
    CHECK_TYPE_SIZE ( WORD _CL_HAVE_TYPE_WORD )

    IF ( _CL_HAVE_TYPE_DWORD )
        CHOOSE_TYPE(_cl_dword_t ${_CL_HAVE_TYPE_DWORD} unsigned "long;long long;__int64;short;int" )
    ENDIF ( _CL_HAVE_TYPE_DWORD )
    
    IF ( _CL_HAVE_TYPE_WORD )
        CHOOSE_TYPE(_cl_word_t ${_CL_HAVE_TYPE_WORD} unsigned "long;long long;__int64;short;int" )
    ENDIF ( _CL_HAVE_TYPE_WORD )
    
    SET ( CMAKE_EXTRA_INCLUDE_FILES )

ENDMACRO ( DEFINE_DWORD ) 