/*
 * This source file is part of MyGUI. For the latest info, see http://mygui.info/
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#include "MyGUI_Precompiled.h"
#include "MyGUI_UString.h"

namespace MyGUI
{


    //--------------------------------------------------------------------------
	UString::_base_iterator::_base_iterator()
	{
		mString = 0;
	}
    
    UString::unicode_char UString::_base_iterator::_getCharacter() const {
        dstring::const_iterator it = mIter;
        return mString->_readUTF32(it);
    }
    
	//--------------------------------------------------------------------------
	void UString::_base_iterator::_moveNext()
	{
        mString->_readUTF32(mIter);
	}
	//--------------------------------------------------------------------------
	//--------------------------------------------------------------------------
	//--------------------------------------------------------------------------
	//--------------------------------------------------------------------------
	UString::_fwd_iterator::_fwd_iterator()
	{

	}
	//--------------------------------------------------------------------------
    UString::_fwd_iterator::_fwd_iterator( const _fwd_iterator& i )
	{
        mIter = i.mIter;
        mString = i.mString;
	}
	//--------------------------------------------------------------------------
	UString::_fwd_iterator& UString::_fwd_iterator::operator++()
	{
        _moveNext();
		return *this;
	}
	//--------------------------------------------------------------------------
	UString::_fwd_iterator UString::_fwd_iterator::operator++( int )
	{
		_fwd_iterator tmp( *this );
        _moveNext();
		return tmp;
	}
	//--------------------------------------------------------------------------
	UString::value_type UString::_fwd_iterator::operator*() const
	{
		return _getCharacter();
	}
    //--------------------------------------------------------------------------
	//--------------------------------------------------------------------------
	//--------------------------------------------------------------------------
	//--------------------------------------------------------------------------
	//--------------------------------------------------------------------------
	//--------------------------------------------------------------------------
	UString::UString()
	{
		_init();
	}
	//--------------------------------------------------------------------------
	UString::UString( const UString& copy )
	{
		_init();
		mData = copy.mData;
	}
    //--------------------------------------------------------------------------
    UString::UString( const unicode_char* str )
    {
        _init();
        assign( str );
    }
	//--------------------------------------------------------------------------
	UString::UString( const UString& str, size_type index, size_type length )
	{
		_init();
		assign( str, index, length );
	}
	//--------------------------------------------------------------------------
	UString::UString( const char* c_str )
	{
		_init();
		assign( c_str );
	}
	//--------------------------------------------------------------------------
	UString::UString( const char* c_str, size_type length )
	{
		_init();
		assign( c_str, length );
	}
	//--------------------------------------------------------------------------
	UString::UString( const std::string& str )
	{
		_init();
		assign( str );
	}
    UString::UString( size_type length, unicode_char c )
    {
        _init();
        assign(length,c);
    }
    UString::UString( UString::iterator begin, UString::iterator end )
    {
        _init();
        assign( begin, end );
    }
	//--------------------------------------------------------------------------
	UString::~UString()
	{
		
	}
	//--------------------------------------------------------------------------
	UString::size_type UString::length() const
	{
        iterator i = begin(), ie = end();
        size_type c = 0;
        while ( i != ie ) {
            ++i;
            ++c;
        }
        return c;
	}
	//--------------------------------------------------------------------------
	void UString::reserve( size_type size )
	{
		mData.reserve( size );
	}
	//--------------------------------------------------------------------------
	void UString::swap( UString& from )
	{
		mData.swap( from.mData );
	}
	//--------------------------------------------------------------------------
	bool UString::empty() const
	{
		return mData.empty();
	}
	//--------------------------------------------------------------------------
	const char* UString::c_str() const
	{
		return asUTF8_c_str();
	}
	
	//--------------------------------------------------------------------------
	void UString::clear()
	{
		mData.clear();
	}
	//--------------------------------------------------------------------------
	UString UString::substr( size_type index, size_type num /*= npos */ ) const
	{
		// this could avoid the extra copy if we used a private specialty constructor
		dstring data = mData.substr( index, num );
		UString tmp;
		tmp.mData.swap( data );
		return tmp;
	}
	//--------------------------------------------------------------------------
	void UString::push_back( unicode_char val )
	{
		char_type cp[16];
		size_t c = _utf32_to_utf8( val, cp );
        mData.append(cp,cp+c);
	}
	
	bool UString::inString( unicode_char ch ) const
	{
		iterator i, ie = end();
		for ( i = begin(); i != ie; ++i ) {
			if ( *i == ch )
				return true;
		}
		return false;
	}


	const char* UString::asUTF8_c_str() const
	{
        return reinterpret_cast<const char*>(mData.c_str());
	}

	UString::iterator UString::begin() const
	{
		iterator i;
		i.mIter = mData.begin();
		i.mString = this;
		return i;
	}


	UString::iterator UString::end() const
	{
		iterator i;
		i.mIter = mData.end();
		i.mString = this;
		return i;
	}

	UString& UString::assign( iterator start, iterator end )
	{
		mData.assign( start.mIter, end.mIter );
		return *this;
	}
    
    UString& UString::assign( size_type count, unicode_char ch ) {
        clear();
        char_type cp[16];
        size_t c = _utf32_to_utf8(ch, cp);
        for (size_t i=0;i<count;++i) {
            mData.append(cp,cp+c);
        }
        return *this;
    }

	UString& UString::assign( const UString& str )
	{
		mData.assign( str.mData );
		return *this;
	}
    
    UString& UString::assign( const unicode_char* str )
    {
        mData.clear();
        char_type cp[16];
        while (*str) {
            size_t c = _utf32_to_utf8( *str, cp );
            mData.append(cp, cp+c);
            ++str;
        }
        return *this;
    }

	
	UString& UString::assign( const UString& str, size_type index, size_type len )
	{
		mData.assign( str.mData, index, len );
		return *this;
	}

	
	UString& UString::assign( const std::string& str )
	{
        size_type len = _verifyUTF8( str );
		clear(); // empty our contents, if there are any
		reserve( len ); // best guess bulk capacity growth
        for (std::string::const_iterator it = str.begin();it!=str.end();++it) {
            mData.push_back(*it);
        }
		return *this;
	}

	UString& UString::assign( const char* c_str )
	{
        size_type len = _verifyUTF8( c_str );
        clear(); // empty our contents, if there are any
        reserve( len ); // best guess bulk capacity growth
        while(*c_str) {
            mData.push_back(*c_str);
            ++c_str;
        }
        return *this;
	}

	UString& UString::assign( const char* c_str, size_type num )
	{
		std::string tmp;
		tmp.assign( c_str, num );
		return assign( tmp );
	}

	UString& UString::append( const UString& str )
	{
		mData.append( str.mData );
		return *this;
	}

	UString& UString::append( const UString& str, size_type index, size_type len )
	{
		mData.append( str.mData, index, len );
		return *this;
	}

	UString& UString::append( iterator start, iterator end )
	{
        MYGUI_DEBUG_ASSERT(start.mString==end.mString, "invalid iterators pair");
		mData.append( start.mIter, end.mIter );
		return *this;
	}

	UString& UString::append( const char* c_str, size_type num )
	{
		UString tmp( c_str, num );
		append( tmp );
		return *this;
	}

	UString& UString::append( size_type num, unicode_char ch )
	{
        char_type cp[16];
        size_t c = _utf32_to_utf8(ch, cp);
        for (size_t i=0;i<num;++i) {
            mData.append(cp,cp+c);
        }
		return *this;
	}
    
    UString::iterator UString::insert( iterator i, const UString& str )
    {
        MYGUI_DEBUG_ASSERT(i.mString==this, "invalid iterator i");
        iterator res;
        res.mString = this;
        size_type pos = i.mIter - static_cast<const dstring&>(mData).begin();
        mData.insert(mData.begin()+pos, str.mData.begin(),str.mData.end());
        res.mIter = mData.begin() + pos + str.mData.length();
        return res;
    }
    UString::iterator UString::insert( iterator i, unicode_char ch )
    {
        MYGUI_DEBUG_ASSERT(check_iterator(i), "invalid iterator i");
        char_type cp[16];
        size_t c = _utf32_to_utf8(ch, cp);
        size_type pos = i.mIter - static_cast<const dstring&>(mData).begin();
        iterator res;
        res.mString = this;
        mData.insert(mData.begin()+pos, cp, cp+c);
        res.mIter = mData.begin() + pos + c;
        return res;
    }
    
	UString::iterator UString::erase( iterator loc )
	{
        MYGUI_DEBUG_ASSERT(check_iterator(loc), "invalid iterator loc");
		iterator ret;
        size_type pos = loc.mIter - static_cast<const dstring&>(mData).begin();
		ret.mIter = mData.erase( mData.begin() + pos );
		ret.mString = this;
		return ret;
	}

	UString::iterator UString::erase( iterator start, iterator end )
	{
        MYGUI_DEBUG_ASSERT(check_iterator(start), "invalid iterator start");
        MYGUI_DEBUG_ASSERT(check_iterator(end), "invalid iterator end");
		iterator ret;
        size_type pos_start = start.mIter - static_cast<const dstring&>(mData).begin();
        size_type pos_end = end.mIter - static_cast<const dstring&>(mData).begin();
		ret.mIter = mData.erase( mData.begin()+pos_start, mData.begin()+pos_end );
		ret.mString = this;
		return ret;
	}

	int UString::compare( const UString& str ) const
	{
		return mData.compare( str.mData );
	}

	
	bool UString::operator<( const UString& right ) const
	{
		return compare( right ) < 0;
	}

	bool UString::operator<=( const UString& right ) const
	{
		return compare( right ) <= 0;
	}

	UString& UString::operator=( const UString& s )
	{
		return assign( s );
	}

	UString& UString::operator=( char ch )
	{
		clear();
		return append( 1, ch );
	}

	UString& UString::operator=( unicode_char ch )
	{
		clear();
		return append( 1, ch );
	}

	bool UString::operator>( const UString& right ) const
	{
		return compare( right ) > 0;
	}

	bool UString::operator>=( const UString& right ) const
	{
		return compare( right ) >= 0;
	}

	bool UString::operator==( const UString& right ) const
	{
		return compare( right ) == 0;
	}

	bool UString::operator!=( const UString& right ) const
	{
		return !operator==( right );
	}

	UString::operator std::string() const 
	{
		return std::string( asUTF8_c_str() );
	}
	
	
	bool UString::_utf8_start_char( unsigned char cp )
	{
		return ( cp & ~_cont_mask ) != _cont;
	}

	size_t UString::_utf8_char_length( unsigned char cp )
	{
		if ( !( cp & 0x80 ) ) return 1;
		if (( cp & ~_lead1_mask ) == _lead1 ) return 2;
		if (( cp & ~_lead2_mask ) == _lead2 ) return 3;
		if (( cp & ~_lead3_mask ) == _lead3 ) return 4;
		if (( cp & ~_lead4_mask ) == _lead4 ) return 5;
		if (( cp & ~_lead5_mask ) == _lead5 ) return 6;

		return 1;
		//throw invalid_data( "invalid UTF-8 sequence header value" );
	}

	size_t UString::_utf8_char_length( unicode_char uc )
	{
		/*
		7 bit:  U-00000000 - U-0000007F: 0xxxxxxx
		11 bit: U-00000080 - U-000007FF: 110xxxxx 10xxxxxx
		16 bit: U-00000800 - U-0000FFFF: 1110xxxx 10xxxxxx 10xxxxxx
		21 bit: U-00010000 - U-001FFFFF: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
		26 bit: U-00200000 - U-03FFFFFF: 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
		31 bit: U-04000000 - U-7FFFFFFF: 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
		*/
		if ( !( uc & ~0x0000007F ) ) return 1;
		if ( !( uc & ~0x000007FF ) ) return 2;
		if ( !( uc & ~0x0000FFFF ) ) return 3;
		if ( !( uc & ~0x001FFFFF ) ) return 4;
		if ( !( uc & ~0x03FFFFFF ) ) return 5;
		if ( !( uc & ~0x7FFFFFFF ) ) return 6;

		return 1;
		//throw invalid_data( "invalid UTF-32 value" );
	}

	size_t UString::_utf8_to_utf32( const unsigned char in_cp[6], unicode_char& out_uc )
	{
		size_t len = _utf8_char_length( in_cp[0] );
		if ( len == 1 ) { // if we are only 1 byte long, then just grab it and exit
			out_uc = in_cp[0];
			return 1;
		}

		unicode_char c = 0; // temporary buffer
		size_t i = 0;
		switch ( len ) { // load header byte
			case 6:
				c = in_cp[i] & _lead5_mask;
				break;
			case 5:
				c = in_cp[i] & _lead4_mask;
				break;
			case 4:
				c = in_cp[i] & _lead3_mask;
				break;
			case 3:
				c = in_cp[i] & _lead2_mask;
				break;
			case 2:
				c = in_cp[i] & _lead1_mask;
				break;
		}

		// load each continuation byte
		for ( ++i; i < len; i++ )
		{
			if (( in_cp[i] & ~_cont_mask ) != _cont )
			{
				//throw invalid_data( "bad UTF-8 continuation byte" );
				out_uc = in_cp[0];
				return 1;
			}
			c <<= 6;
			c |= ( in_cp[i] & _cont_mask );
		}

		out_uc = c; // write the final value and return the used byte length
		return len;
	}

	size_t UString::_utf32_to_utf8( const unicode_char& in_uc, unsigned char out_cp[6] )
	{
		size_t len = _utf8_char_length( in_uc ); // predict byte length of sequence
		unicode_char c = in_uc; // copy to temp buffer

		//stuff all of the lower bits
		for ( size_t i = len - 1; i > 0; i-- ) {
			out_cp[i] = static_cast<unsigned char>((( c ) & _cont_mask ) | _cont);
			c >>= 6;
		}

		//now write the header byte
		switch ( len ) {
			case 6:
				out_cp[0] = static_cast<unsigned char>((( c ) & _lead5_mask ) | _lead5);
				break;
			case 5:
				out_cp[0] = static_cast<unsigned char>((( c ) & _lead4_mask ) | _lead4);
				break;
			case 4:
				out_cp[0] = static_cast<unsigned char>((( c ) & _lead3_mask ) | _lead3);
				break;
			case 3:
				out_cp[0] = static_cast<unsigned char>((( c ) & _lead2_mask ) | _lead2);
				break;
			case 2:
				out_cp[0] = static_cast<unsigned char>((( c ) & _lead1_mask ) | _lead1);
				break;
			case 1:
			default:
				out_cp[0] = static_cast<unsigned char>(( c ) & 0x7F);
				break;
		}

		// return the byte length of the sequence
		return len;
	}
    
    UString::unicode_char UString::_readUTF32(dstring::const_iterator &it) const {
        UString::unicode_char res = 0;
        dstring::const_iterator ie = mData.end();
        if (it == ie) return res;
        size_t len = _utf8_char_length(*it);
        if ( len == 1 ) { // if we are only 1 byte long, then just grab it and exit
            res = *it;
            ++it;
            return res;
        }
        
        size_t i = 0;
        switch ( len ) { // load header byte
            case 6:
                res = *it & _lead5_mask;
                break;
            case 5:
                res = *it & _lead4_mask;
                break;
            case 4:
                res = *it & _lead3_mask;
                break;
            case 3:
                res = *it & _lead2_mask;
                break;
            case 2:
                res = *it & _lead1_mask;
                break;
        }
        ++it;
        if (it == ie)
            return res;
        // load each continuation byte
        for ( ++i; i < len; i++ )
        {
            if (( *it & ~_cont_mask ) != _cont )
            {
                //throw invalid_data( "bad UTF-8 continuation byte" );
                return res;
            }
            res <<= 6;
            res |= ( *it & _cont_mask );
            ++it;
            if (it == ie)
                break;
        }
        
        return res;
    }

	UString::size_type UString::_verifyUTF8( const unsigned char* c_str )
	{
		std::string tmp( reinterpret_cast<const char*>( c_str ) );
		return _verifyUTF8( tmp );
	}

	UString::size_type UString::_verifyUTF8( const std::string& str )
	{
		std::string::const_iterator i, ie = str.end();
		i = str.begin();
		size_type length = 0;

		while ( i != ie ) {
			// characters pass until we find an extended sequence
			if (( *i ) & 0x80 ) {
				unsigned char c = ( *i );
				size_t contBytes = 0;

				// get continuation byte count and test for overlong sequences
				if (( c & ~_lead1_mask ) == _lead1 ) { // 1 additional byte
					if ( c == _lead1 )
					{
						//throw invalid_data( "overlong UTF-8 sequence" );
						return str.size();
					}
					contBytes = 1;

				} else if (( c & ~_lead2_mask ) == _lead2 ) { // 2 additional bytes
					contBytes = 2;
					if ( c == _lead2 ) { // possible overlong UTF-8 sequence
						c = ( *( i + 1 ) ); // look ahead to next byte in sequence
						if (( c & _lead2 ) == _cont )
						{
							//throw invalid_data( "overlong UTF-8 sequence" );
							return str.size();
						}
					}

				} else if (( c & ~_lead3_mask ) == _lead3 ) { // 3 additional bytes
					contBytes = 3;
					if ( c == _lead3 ) { // possible overlong UTF-8 sequence
						c = ( *( i + 1 ) ); // look ahead to next byte in sequence
						if (( c & _lead3 ) == _cont )
						{
							//throw invalid_data( "overlong UTF-8 sequence" );
							return str.size();
						}
					}

				} else if (( c & ~_lead4_mask ) == _lead4 ) { // 4 additional bytes
					contBytes = 4;
					if ( c == _lead4 ) { // possible overlong UTF-8 sequence
						c = ( *( i + 1 ) ); // look ahead to next byte in sequence
						if (( c & _lead4 ) == _cont )
						{
							//throw invalid_data( "overlong UTF-8 sequence" );
							return str.size();
						}
					}

				} else if (( c & ~_lead5_mask ) == _lead5 ) { // 5 additional bytes
					contBytes = 5;
					if ( c == _lead5 ) { // possible overlong UTF-8 sequence
						c = ( *( i + 1 ) ); // look ahead to next byte in sequence
						if (( c & _lead5 ) == _cont )
						{
							//throw invalid_data( "overlong UTF-8 sequence" );
							return str.size();
						}
					}
				}

				// check remaining continuation bytes for
				while ( contBytes-- ) {
					c = ( *( ++i ) ); // get next byte in sequence
					if (( c & ~_cont_mask ) != _cont )
					{
						//throw invalid_data( "bad UTF-8 continuation byte" );
						return str.size();
					}
				}
			}
			length++;
			i++;
		}
		return length;
	}
    
    bool UString::check_iterator( const _base_iterator& i ) const {
        MYGUI_ASSERT(i.mString == this, "not my iterator");
        for (dstring::const_iterator it = mData.begin();it!=mData.end();++it) {
            if (it == i.mIter)
                return true;
        }
        return i.mIter==mData.end();
    }

	void UString::_init()
	{
		
	}

} // namespace MyGUI
