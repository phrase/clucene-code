/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#ifndef _lucene_document_ComparableTools_
#define _lucene_document_ComparableTools_

#include <string>
#include <utility>

CL_NS_DEF(document)

	#define NUMBERTOOLS_RADIX 36

	#define NEGATIVE_PREFIX _T("-")
	// NB: NEGATIVE_PREFIX must be < POSITIVE_PREFIX
	#define POSITIVE_PREFIX _T("0")

/**
 * Provides support for converting numerically based comparable types to
 * Strings, and back again. The strings
 * are structured so that lexicographic sorting order is preserved.
 * 
 * <p>
 * That is, if l1 is less than l2 for any two longs l1 and l2, then
 * ComparableTools.valueToString(l1) is lexicographically less than
 * ComparableTools.valueToString(l2). (Similarly for "greater than" and "equals".)
 * 
 */
template<typename comparable>
class CLUCENE_EXPORT ComparableTools :LUCENE_BASE {

public:
	/**
     * Converts a long to a String suitable for indexing.
     */
    static std::basic_string<TCHAR> valueToString(comparable l);

    /**
     * Converts a String that was returned by {@link #valueToString} back to a
     * long.
     * 
     * @throws IllegalArgumentException
     *             if the input is null
     * @throws NumberFormatException
     *             if the input does not parse (it was not a String returned by
     *             valueToString()).
     */
    static comparable stringToValue(std::basic_string<TCHAR> const& str);

	~ComparableTools();

};

template<>
class CLUCENE_EXPORT ComparableTools<int64_t> :LUCENE_BASE {

public:
    /**
     * Equivalent to valueToString(Long.MIN_VALUE); STR_SIZE is depandant on the length of it
     */
	static inline std::basic_string<TCHAR> MIN_STRING_VALUE() {
		return NEGATIVE_PREFIX _T("0000000000000");
	}

	/**
     * Equivalent to valueToString(Long.MAX_VALUE)
     */
	static inline std::basic_string<TCHAR> MAX_STRING_VALUE() {
		return POSITIVE_PREFIX _T("1y2p0ij32e8e7");
	}

	/**
     * The length of (all) strings returned by {@link #valueToString}
     */
    LUCENE_STATIC_CONSTANT (size_t, STR_SIZE = 14);

	/**
     * Converts a long to a String suitable for indexing.
     */
    static std::basic_string<TCHAR> valueToString(int64_t l);

    /**
     * Converts a String that was returned by {@link #valueToString} back to a
     * long.
     * 
     * @throws IllegalArgumentException
     *             if the input is null
     * @throws NumberFormatException
     *             if the input does not parse (it was not a String returned by
     *             valueToString()).
     */
    static int64_t stringToValue(std::basic_string<TCHAR> const& str);

	~ComparableTools();

};

template<typename first_type,typename second_type>
class CLUCENE_EXPORT ComparableTools<std::pair<first_type,second_type> > :LUCENE_BASE {

public:
	static inline std::basic_string<TCHAR> MIN_STRING_VALUE() {
		return ComparableTools<first_type>::MIN_STRING_VALUE() + ComparableTools<second_type>::MIN_STRING_VALUE();
	}

	static inline std::basic_string<TCHAR> MAX_STRING_VALUE() {
		return ComparableTools<first_type>::MAX_STRING_VALUE() + ComparableTools<second_type>::MAX_STRING_VALUE();
	}

	/**
     * The length of (all) strings returned by {@link #valueToString}
     */
    LUCENE_STATIC_CONSTANT (size_t, STR_SIZE = ComparableTools<first_type>::STR_SIZE + ComparableTools<second_type>::STR_SIZE);

	/**
     * Converts a long to a String suitable for indexing.
     */
    static std::basic_string<TCHAR> valueToString(std::pair<first_type,second_type> l);

    /**
     * Converts a String that was returned by {@link #valueToString} back to a
     * long.
     * 
     * @throws IllegalArgumentException
     *             if the input is null
     * @throws NumberFormatException
     *             if the input does not parse (it was not a String returned by
     *             valueToString()).
     */
    static std::pair<first_type,second_type> stringToValue(std::basic_string<TCHAR> const& str);

	~ComparableTools();

};

template<typename first_type,typename second_type>
std::basic_string<TCHAR> ComparableTools<std::pair<first_type,second_type> >::valueToString(std::pair<first_type,second_type> l) {
	return ComparableTools<first_type>::valueToString(l.first) + ComparableTools<second_type>::valueToString(l.second);
}

template<typename first_type,typename second_type>
std::pair<first_type,second_type> ComparableTools<std::pair<first_type,second_type> >::stringToValue(std::basic_string<TCHAR> const& str) {
	return std::pair<first_type,second_type>(
		ComparableTools<first_type>::stringToValue(
			std::basic_string<TCHAR>(
				str.c_str(),
				str.c_str() + ComparableTools<first_type>::STR_SIZE
			)
		),
		ComparableTools<second_type>::stringToValue(
			std::basic_string<TCHAR>(
				str.c_str() + ComparableTools<first_type>::STR_SIZE
			)
		)
	);
}

template<typename first_type,typename second_type>
ComparableTools<std::pair<first_type,second_type> >::~ComparableTools() {
}
CL_NS_END
#endif
