/*------------------------------------------------------------------------------
* Copyright (C) 2003-2006 Ben van Klinken and the CLucene Team
* 
* Distributable under the terms of either the Apache License (Version 2.0) or 
* the GNU Lesser General Public License, as specified in the COPYING file.
------------------------------------------------------------------------------*/
#include "CLucene/_ApiHeader.h"
#include "AnalysisHeader.h"
#include "CLucene/util/StringBuffer.h"
#include "CLucene/util/_ThreadLocal.h"

CL_NS_USE(util)
CL_NS_DEF(analysis)

struct Analyzer::Internal{
	CL_NS(util)::ThreadLocal<TokenStream*,
		CL_NS(util)::Deletor::Object<TokenStream> >* tokenStreams;
};
Analyzer::Analyzer(){
	internal = new Internal;
	internal->tokenStreams = _CLNEW CL_NS(util)::ThreadLocal<TokenStream*,
		CL_NS(util)::Deletor::Object<TokenStream> >;
}
Analyzer::~Analyzer(){
	delete internal;
}
TokenStream* Analyzer::getPreviousTokenStream() {
	return internal->tokenStreams->get();
}
void Analyzer::setPreviousTokenStream(TokenStream* obj) {
	internal->tokenStreams->set(obj);
}
TokenStream* Analyzer::reusableTokenStream(const TCHAR* fieldName, CL_NS(util)::Reader* reader) {
	return tokenStream(fieldName, reader);
}

///Compares the Token for their order
class OrderCompare:LUCENE_BASE, public CL_NS(util)::Compare::_base //<Token*>
{
public:
	bool operator()( Token* t1, Token* t2 ) const{
	if(t1->startOffset()>t2->startOffset())
        return false;
    if(t1->startOffset()<t2->startOffset())
        return true;
	return true;
}
};

Token::Token():
	_startOffset (0),
	_endOffset (0),
	_type ( getDefaultType() ),
	positionIncrement (1),
	payload(NULL)
{
    _termTextLen = 0;
#ifndef LUCENE_TOKEN_WORD_LENGTH
    _termText = NULL;
	bufferTextLen = 0;
#else
    _termText[0] = 0; //make sure null terminated
	bufferTextLen = LUCENE_TOKEN_WORD_LENGTH+1;
#endif
}

Token::~Token(){
#ifndef LUCENE_TOKEN_WORD_LENGTH
    free(_termText);
#endif
	_CLLDELETE(payload);
}

Token::Token(const TCHAR* text, const int32_t start, const int32_t end, const TCHAR* typ):
	_startOffset (start),
	_endOffset (end),
	_type ( (typ==NULL?getDefaultType():typ) ),
	positionIncrement (1),
	payload(NULL)
{
    _termTextLen = 0;
#ifndef LUCENE_TOKEN_WORD_LENGTH
    _termText = NULL;
	bufferTextLen = 0;
#else
    _termText[0] = 0; //make sure null terminated
	bufferTextLen = LUCENE_TOKEN_WORD_LENGTH+1;
#endif
	setText(text);
}

const TCHAR* Token::getDefaultType(){
    return _T("word");
}

void Token::set(const TCHAR* text, const int32_t start, const int32_t end, const TCHAR* typ){
	_startOffset = start;
	_endOffset   = end;
	_type        = (typ==NULL?getDefaultType():typ);
	positionIncrement = 1;
	setText(text);
}

void Token::setText(const TCHAR* text){
	_termTextLen = _tcslen(text);
	
#ifndef LUCENE_TOKEN_WORD_LENGTH
	growBuffer(_termTextLen+1);
	_tcsncpy(_termText,text,_termTextLen+1);
#else
	if ( _termTextLen > LUCENE_TOKEN_WORD_LENGTH ){
    	//in the case where this occurs, we will leave the endOffset as it is
    	//since the actual word still occupies that space.
		_termTextLen=LUCENE_TOKEN_WORD_LENGTH;
	}
	_tcsncpy(_termText,text,_termTextLen+1);
#endif
	_termText[_termTextLen] = 0; //make sure null terminated
}

void Token::growBuffer(size_t size){
	if(bufferTextLen>=size)
		return;
#ifndef LUCENE_TOKEN_WORD_LENGTH
	if ( _termText == NULL )
		_termText = (TCHAR*)malloc( size * sizeof(TCHAR) );
	else
		_termText = (TCHAR*)realloc( _termText, size * sizeof(TCHAR) );
	bufferTextLen = size;
#else
	_CLTHROWA(CL_ERR_TokenMgr,"Couldn't grow Token buffer");
#endif
}

void Token::setPositionIncrement(int32_t posIncr) {
	if (posIncr < 0) {
		_CLTHROWA(CL_ERR_IllegalArgument,"positionIncrement must be >= 0");
	}
	positionIncrement = posIncr;
}

int32_t Token::getPositionIncrement() const { return positionIncrement; }

// Returns the Token's term text. 
const TCHAR* Token::termText() const{
	return (const TCHAR*) _termText; 
}
const TCHAR* Token::termBuffer() const{
	return (const TCHAR*) _termText; 
}
size_t Token::termTextLength() { 
	if ( _termTextLen == -1 ) //it was invalidated by growBuffer
		_termTextLen = _tcslen(_termText);
	return _termTextLen; 
}
size_t Token::termLength() { 
	if ( _termTextLen == -1 ) //it was invalidated by growBuffer
		_termTextLen = _tcslen(_termText);
	return _termTextLen; 
}
void Token::resetTermTextLen(){
	_termTextLen=-1;
}
TCHAR* Token::toString() const{
	StringBuffer sb;
    sb.append(_T("("));
	if (_termText)
		sb.append( _termText );
	else
		sb.append( _T("null") );
    sb.append(_T(","));
    sb.appendInt( _startOffset );
    sb.append(_T(","));
    sb.appendInt( _endOffset );
    
    if (!_tcscmp( _type, _T("word")) == 0 ){
      sb.append(_T(",type="));
      sb.append(_type);
    }
    if (positionIncrement != 1){
      sb.append(_T(",posIncr="));
      sb.appendInt(positionIncrement);
    }
    sb.append(_T(")"));

    return sb.toString();
}

CL_NS(index)::Payload* Token::getPayload() { return this->payload; }
void Token::setPayload(CL_NS(index)::Payload* payload) {
	_CLLDELETE(this->payload);
	this->payload = payload;
}
void Token::clear() {
	_CLDELETE(payload);
	// Leave _termText to allow re-use
	_termTextLen = 0;
	positionIncrement = 1;
	// startOffset = endOffset = 0;
	// type = DEFAULT_TYPE;
}


Token* TokenStream::next(){
	Token* t = _CLNEW Token; //deprecated
	if ( !next(t) )
		_CLDELETE(t);
	return t;
}


TokenFilter::TokenFilter(TokenStream* in, bool deleteTS):
	input(in),
	deleteTokenStream(deleteTS)
{
}
TokenFilter::~TokenFilter(){
	close();
}

// Close the input TokenStream.
void TokenFilter::close() {
    if ( input != NULL ){
		input->close();
        if ( deleteTokenStream )
			_CLDELETE( input );
    }
    input = NULL;
}



Tokenizer::Tokenizer() {
	input = NULL;
}

Tokenizer::Tokenizer(CL_NS(util)::Reader* _input):
    input(_input)
{
}

void Tokenizer::close(){
	if (input != NULL) {
		// ? delete input;
		input = NULL;
	}
}

void Tokenizer::reset(CL_NS(util)::Reader* _input) {
	// ? delete input;
	this->input = _input;
}

Tokenizer::~Tokenizer(){ 
    close();
}


int32_t Analyzer::getPositionIncrementGap(const TCHAR* fieldName)
{
	return 0;
}

CL_NS_END
