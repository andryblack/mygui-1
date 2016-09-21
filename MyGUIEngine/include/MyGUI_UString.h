// Modified from OpenGUI under lenient license
// Original copyright details and licensing below:
// OpenGUI (http://opengui.sourceforge.net)
// This source code is released under the BSD License

// Permission is given to the MyGUI project to use the contents of file within its
// source and binary applications, as well as any derivative works, in accordance
// with the terms of any license under which MyGUI is or will be distributed.
//
// MyGUI may relicense its copy of this file, as well as any OpenGUI released updates
// to this file, under any terms that it deems fit, and is not required to maintain
// the original BSD licensing terms of this file, however OpenGUI retains the right
// to present its copy of this file under the terms of any license under which
// OpenGUI is distributed.
//
// MyGUI is not required to release to OpenGUI any future changes that it makes to
// this file, and understands and agrees that any such changes that are released
// back to OpenGUI will become available under the terms of any license under which
// OpenGUI is distributed.
//
// For brevity, this permission text may be removed from this file if desired.
// The original record kept within the SourceForge (http://sourceforge.net/) tracker
// is sufficient.
//
// - Eric Shorkey (zero/zeroskill) <opengui@rightbracket.com> [January 20th, 2007]

#ifndef MYGUI_U_STRING_H_
#define MYGUI_U_STRING_H_


#include "MyGUI_Prerequest.h"
#include "MyGUI_Types.h"

// these are explained later
#include <iterator>
#include <string>
#include <stdexcept>

#if MYGUI_COMPILER == MYGUI_COMPILER_MSVC
// disable: warning C4275: non dll-interface class '***' used as base for dll-interface clas '***'
#	pragma warning (push)
#	pragma warning (disable : 4275)
#endif



namespace MyGUI
{

	/* READ THIS NOTICE BEFORE USING IN YOUR OWN APPLICATIONS
	=NOTICE=
	This class is not a complete Unicode solution. It purposefully does not
	provide certain functionality, such as proper lexical sorting for
	Unicode values. It does provide comparison operators for the sole purpose
	of using UString as an index with std::map and other operator< sorted
	containers, but it should NOT be relied upon for meaningful lexical
	operations, such as alphabetical sorts. If you need this type of
	functionality, look into using ICU instead (http://icu.sourceforge.net/).

	=REQUIREMENTS=
	There are a few requirements for proper operation. They are fairly small,
	and shouldn't restrict usage on any reasonable target.
	* Compiler must support unsigned 16-bit integer types
	* Compiler must support signed 32-bit integer types
	* You must include <iterator>, <string>, and <wchar>. Probably more, but
	    these are the most obvious.

	=REQUIRED PREPROCESSOR MACROS=
	This class requires two preprocessor macros to be defined in order to
	work as advertised.
	INT32 - must be mapped to a signed 32 bit integer (ex. #define INT32 int)
	UINT16 - must be mapped to an unsigned 16 bit integer (ex. #define UINT32 unsigned short)

	*/

	
	//! A UTF-16 string with implicit conversion to/from std::string and std::wstring
	/*! This class provides a complete 1 to 1 map of most std::string functions (at least to my
	knowledge). Implicit conversions allow this string class to work with all common C++ string
	formats, with specialty functions defined where implicit conversion would cause potential
	problems or is otherwise unavailable.

	Some additional functionality is present to assist in working with characters using the
	32-bit UTF-32 encoding. (Which is guaranteed to fit any Unicode character into a single
	code point.) \b Note: Reverse iterators do not have this functionality due to the
	ambiguity that surrounds working with UTF-16 in reverse. (Such as, where should an
	iterator point to represent the beginning of a surrogate pair?)


	\par Supported Input Types
	The supported string types for input, and their assumed encoding schemes, are:
	- std::string (UTF-8)
	- char* (UTF-8)
	

	\see
	- For additional information on UTF-16 encoding: http://en.wikipedia.org/wiki/UTF-16
	- For additional information on UTF-8 encoding: http://en.wikipedia.org/wiki/UTF-8
	- For additional information on UTF-32 encoding: http://en.wikipedia.org/wiki/UTF-32
	*/
	class MYGUI_EXPORT UString {
		// constants used in UTF-8 conversions
		static const unsigned char _lead1 = 0xC0;      //110xxxxx
		static const unsigned char _lead1_mask = 0x1F; //00011111
		static const unsigned char _lead2 = 0xE0;      //1110xxxx
		static const unsigned char _lead2_mask = 0x0F; //00001111
		static const unsigned char _lead3 = 0xF0;      //11110xxx
		static const unsigned char _lead3_mask = 0x07; //00000111
		static const unsigned char _lead4 = 0xF8;      //111110xx
		static const unsigned char _lead4_mask = 0x03; //00000011
		static const unsigned char _lead5 = 0xFC;      //1111110x
		static const unsigned char _lead5_mask = 0x01; //00000001
		static const unsigned char _cont = 0x80;       //10xxxxxx
		static const unsigned char _cont_mask = 0x3F;  //00111111

	public:
		//! size type used to indicate string size and character positions within the string
		typedef size_t size_type;
		//! the usual constant representing: not found, no limit, etc
		static const size_type npos = static_cast<size_type>(~0);

		//! a single 32-bit Unicode character
		typedef uint32 unicode_char;

        //! value type typedef for use in iterators
		typedef unicode_char value_type;

        typedef uint8 char_type;
        
		typedef std::basic_string<char_type> dstring; // data string

		//! string type used for returning UTF-32 formatted data
		typedef std::basic_string<unicode_char> utf32string;

		
		//#########################################################################
		//! base iterator class for UString
	class MYGUI_EXPORT _base_iterator: public std::iterator<std::forward_iterator_tag, value_type> { /* i don't know why the beautifier is freaking out on this line */
			friend class UString;
		protected:
			_base_iterator();

            unicode_char _getCharacter() const;
			
            void _moveNext();
			
            dstring::const_iterator mIter;
			const UString* mString;
		};

		//#########################################################################
		// FORWARD ITERATORS
		//#########################################################################
		class _const_fwd_iterator; // forward declaration

		//! forward iterator for UString
        class MYGUI_EXPORT _fwd_iterator: public _base_iterator {
        public:
			_fwd_iterator();
			_fwd_iterator( const _fwd_iterator& i );

			//! pre-increment
			_fwd_iterator& operator++();
			//! post-increment
			_fwd_iterator operator++( int );

			
            //! dereference operator
			value_type operator*() const;

            bool operator == (const _fwd_iterator& it) const {
                return mIter == it.mIter;
            }
            bool operator != (const _fwd_iterator& it) const {
                return mIter != it.mIter;
            }
        };

		
		typedef _fwd_iterator iterator;                     //!< iterator
		

		//!\name Constructors/Destructor
		//@{
		//! default constructor, creates an empty string
		UString();
		//! copy constructor
		UString( const UString& copy );
		//! duplicate of nul-terminated sequence \a str
        UString( const unicode_char* str );
		//! substring of \a str starting at \a index and \a length code points long
		UString( const UString& str, size_type index, size_type length );
		//! duplicate of nul-terminated C-string \a c_str (UTF-8 encoding)
		UString( const char* c_str );
		//! duplicate of \a c_str, \a length characters long (UTF-8 encoding)
		UString( const char* c_str, size_type length );
		//! duplicate of \a str (UTF-8 encoding)
		UString( const std::string& str );
        //! duplicate of \a c (UTF-32 encoding)
        UString( size_type length, unicode_char c );
        //! copy constructor
        UString( iterator begin, iterator end );
		//! destructor
		~UString();
		//@}

		//////////////////////////////////////////////////////////////////////////

		//!\name Utility functions
		//@{
		//! Returns the number of code points in the current string
		size_type length() const;
		//! sets the capacity of the string to at least \a size code points
		void reserve( size_type size );
		//! changes the size of the string to \a size, filling in any new area with \a val
		void resize( size_type num, const unicode_char& val = 0 );
		//! exchanges the elements of the current string with those of \a from
		void swap( UString& from );
		//! returns \c true if the string has no elements, \c false otherwise
		bool empty() const;
		//! returns a pointer to the first character in the current string
		const char* c_str() const;
		//! returns a pointer to the first character in the current string
        const dstring& data() const { return mData; }
		//! deletes all of the elements in the string
		void clear();
		//! returns a substring of the current string, starting at \a index, and \a num characters long.
		/*! If \a num is omitted, it will default to \c UString::npos, and the substr() function will simply return the remainder of the string starting at \a index. */
		UString substr( size_type index, size_type num = npos ) const;
		//! appends \a val to the end of the string
		void push_back( unicode_char val );
		//! returns \c true if the given Unicode character \a ch is in this string
		bool inString( unicode_char ch ) const;
		//@}

		//////////////////////////////////////////////////////////////////////////

		//!\name Stream variations
		//@{
		//! returns the current string in UTF-8 form as a nul-terminated char array
		const char* asUTF8_c_str() const;
		//@}


		//////////////////////////////////////////////////////////////////////////

		//!\name iterator acquisition
		//@{
		//! returns an iterator to the first element of the string
		iterator begin() const;
		//! returns an iterator just past the end of the string
		iterator end() const;
		//@}

		//////////////////////////////////////////////////////////////////////////

		//!\name assign
		//@{
		//! gives the current string the values from \a start to \a end
		UString& assign( iterator start, iterator end );
		//! assign \a str to the current string
		UString& assign( const UString& str );
        //! assign \a c to the current string
        UString& assign( size_type count, unicode_char ch );
        //! assign the nul-terminated \a str to the current string
        UString& assign( const unicode_char* str );
		//! assign \a len entries from \a str to the current string, starting at \a index
		UString& assign( const UString& str, size_type index, size_type len );
		//! assign \a str to the current string (\a str is treated as a UTF-8 stream)
		UString& assign( const std::string& str );
		//! assign \a c_str to the current string (\a c_str is treated as a UTF-8 stream)
		UString& assign( const char* c_str );
		//! assign the first \a num characters of \a c_str to the current string (\a c_str is treated as a UTF-8 stream)
		UString& assign( const char* c_str, size_type num );
		//@}

		//////////////////////////////////////////////////////////////////////////

		//!\name append
		//@{
		//! appends \a str on to the end of the current string
		UString& append( const UString& str );
		//! appends a substring of \a str starting at \a index that is \a len characters long on to the end of the current string
		UString& append( const UString& str, size_type index, size_type len );
		//! appends the sequence denoted by \a start and \a end on to the end of the current string
		UString& append( iterator start, iterator end );
		//! appends \a num characters of \a str on to the end of the current string  (UTF-8 encoding)
		UString& append( const char* c_str, size_type num );
		//! appends \a num repetitions of \a ch on to the end of the current string (Unicode values less than 128)
		UString& append( size_type num, char ch );
		//! appends \a num repetitions of \a ch on to the end of the current string (Full Unicode spectrum)
		UString& append( size_type num, unicode_char ch );
		//@}

		//////////////////////////////////////////////////////////////////////////

		//!\name insert
		//@{
		iterator insert( iterator i, const UString& str );
        iterator insert( iterator i, unicode_char ch );
		//@}

		//////////////////////////////////////////////////////////////////////////

		//!\name erase
		//@{
		//! removes the code point pointed to by \a loc, returning an iterator to the next character
		iterator erase( iterator loc );
		//! removes the code points between \a start and \a end (including the one at \a start but not the one at \a end), returning an iterator to the code point after the last code point removed
		iterator erase( iterator start, iterator end );
		//@}

		//////////////////////////////////////////////////////////////////////////

		//!\name replace
		//@{
		//! replaces code points in the current string from \a start to \a end with \a num code points from \a str
		iterator replace( iterator start, iterator end, const UString& str );
        iterator replace( iterator start, unicode_char ch );
			//@}

		//////////////////////////////////////////////////////////////////////////

		//!\name compare
		//@{
		//! compare \a str to the current string
		int compare( const UString& str ) const;
		//@}

		//////////////////////////////////////////////////////////////////////////

		//!\name Operators
		//@{
		//! less than operator
		bool operator<( const UString& right ) const;
		//! less than or equal operator
		bool operator<=( const UString& right ) const;
		//! greater than operator
		bool operator>( const UString& right ) const;
		//! greater than or equal operator
		bool operator>=( const UString& right ) const;
		//! equality operator
		bool operator==( const UString& right ) const;
		//! inequality operator
		bool operator!=( const UString& right ) const;
		//! assignment operator, implicitly casts all compatible types
		UString& operator=( const UString& s );
		//! assignment operator
		UString& operator=( char ch );
		//! assignment operator
		UString& operator=( unicode_char ch );
		//@}

		//////////////////////////////////////////////////////////////////////////

		//!\name Implicit Cast Operators
		//@{
		//! implicit cast to std::string
		operator std::string() const;
		//@}

		//////////////////////////////////////////////////////////////////////////

		
		//////////////////////////////////////////////////////////////////////////

		//!\name UTF-8 character encoding/decoding
		//@{
		//! returns \c true if \a cp is the beginning of a UTF-8 sequence
		static bool _utf8_start_char( char_type cp );
		//! estimates the number of UTF-8 code points in the sequence starting with \a cp
		static size_t _utf8_char_length( char_type cp );
		//! returns the number of UTF-8 code points needed to represent the given UTF-32 character \a cp
		static size_t _utf8_char_length( unicode_char uc );

		//! converts the given UTF-8 character buffer to a single UTF-32 Unicode character, returns the number of bytes used to create the output character (maximum of 6)
		static size_t _utf8_to_utf32( const char_type in_cp[6], unicode_char& out_uc );
		//! writes the given UTF-32 \a uc_in to the buffer location \a out_cp using UTF-8 encoding, returns the number of bytes used to encode the input
		static size_t _utf32_to_utf8( const unicode_char& in_uc, char_type out_cp[6] );

		//! verifies a UTF-8 stream, returning the total number of Unicode characters found
		static size_type _verifyUTF8( const char_type* c_str );
		//! verifies a UTF-8 stream, returning the total number of Unicode characters found
		static size_type _verifyUTF8( const std::string& str );
		//@}

        unicode_char _readUTF32(dstring::const_iterator& it) const;
	private:
		//template<class ITER_TYPE> friend class _iterator;
		dstring mData;

		//! common constructor operations
		void _init();
	};

	//! string addition operator \relates UString
	inline UString operator+( const UString& s1, const UString& s2 ) {
		return UString( s1 ).append( s2 );
	}
	//! string addition operator \relates UString
	inline UString operator+( const UString& s1, UString::unicode_char c ) {
		return UString( s1 ).append( 1, c );
	}
	//! string addition operator \relates UString
	inline UString operator+( const UString& s1, char c ) {
		return UString( s1 ).append( 1, c );
	}
	//! string addition operator \relates UString
	inline UString operator+( UString::unicode_char c, const UString& s2 ) {
		return UString().append( 1, c ).append( s2 );
	}
	//! string addition operator \relates UString
	inline UString operator+( char c, const UString& s2 ) {
		return UString().append( 1, c ).append( s2 );
	}
		
	
	//! std::ostream write operator \relates UString
	inline std::ostream& operator << ( std::ostream& os, const UString& s ) {
		return os << s.asUTF8_c_str();
	}


} // namespace MyGUI

#if MYGUI_COMPILER == MYGUI_COMPILER_MSVC
#	pragma warning (pop)
#endif

#endif  // __MYGUI_U_STRING_H__
