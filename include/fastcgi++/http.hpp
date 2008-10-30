//! \file http.hpp Defines elements of the HTTP protocol
/***************************************************************************
* Copyright (C) 2007 Eddie                                                 *
*                                                                          *
* This file is part of fastcgi++.                                          *
*                                                                          *
* fastcgi++ is free software: you can redistribute it and/or modify it     *
* under the terms of the GNU Lesser General Public License as  published   *
* by the Free Software Foundation, either version 3 of the License, or (at *
* your option) any later version.                                          *
*                                                                          *
* fastcgi++ is distributed in the hope that it will be useful, but WITHOUT *
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or    *
* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public     *
* License for more details.                                                *
*                                                                          *
* You should have received a copy of the GNU Lesser General Public License *
* along with fastcgi++.  If not, see <http://www.gnu.org/licenses/>.       *
****************************************************************************/


#ifndef HTTP_HPP
#define HTTP_HPP

#include <string>
#include <boost/shared_array.hpp>
#include <boost/scoped_array.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <ostream>
#include <istream>
#include <cstring>
#include <sstream>
#include <map>

#include <fastcgi++/exceptions.hpp>
#include <fastcgi++/protocol.hpp>

//! Topmost namespace for the fastcgi++ library
namespace Fastcgipp
{
	//! Defines classes and function relating to the http protocol
	namespace Http
	{
		//! Holds a piece of HTTP post data
		/*!
		 * This structure will hold one of two types of HTTP post data. It can
		 * either contain form data, in which case the data field is empty and
		 * the size is zero; or it can hold an uploaded file, in which case data
		 * contains a pointer to the file data, size contains it's size and value holds it's
		 * filename. The actual name associated with the piece of post data
		 * is omitted from the class so it can be linked in an associative
		 * container.
		 *
		 * @tparam charT Type of character to use in the value string (char or wchar_t)
		 */
		template<class charT> struct Post
		{
			//! Type of POST data piece
			enum Type { file, form } type;
			//! Value of POST data if type=form or the filename if type=file
			std::basic_string<charT> value;
			//! Pointer to file data
			boost::shared_array<char> data;
			//! Size of data in bytes pointed to by data.
			size_t size;
		};

		//! Efficiently stores IPv4 addresses
		/*!
		 * This class stores IPv4 addresses as unsigned 32bit integers. It does this
		 * as opposed to storing the string itself to facilitate efficient logging and
		 * processing of the address. The class possesses full IO and comparison capabilities
		 * as well as allowing bitwise AND operations for netmask calculation.
		 */
		class Address
		{
		public:
			//! Retrieve the integer value of the IPv4 address
			/*!
			 * @return Unsigned 32bit integer representing the IPv4 address
			 */
			uint32_t getInt() const { return data; }
			//! Assign the IPv4 address from an integer
			/*!
			 * @param[in] data_ Unsigned 32bit integer representing the IPv4 address
			 */
			Address operator=(uint32_t data_) { data=data_; return *this; }
			Address operator=(Address address) { data=address.data; return *this; }
			Address(const Address& address): data(address.data) { }
			//! Construct the IPv4 address from an integer
			/*!
			 * @param[in] data_ Unsigned 32bit integer representing the IPv4 address
			 */
			Address(uint32_t data_): data(data_) { }
			//! Constructs from a value of 0.0.0.0 (0)
			Address(): data(0) { }
			//! Assign the IPv4 address from a string of characters
			/*!
			 * In order for this to work the string must represent an IPv4 address in
			 * textual decimal form and nothing else. Example: "127.0.0.1".
			 *
			 * @param[in] start First character of the string
			 * @param[in] end Last character of the string + 1
			 */
			void assign(const char* start, const char* end);
		private:
			friend inline bool operator==(Address x, Address y);
			friend inline bool operator>(Address x, Address y);
			friend inline bool operator<(Address x, Address y);
			friend inline bool operator<=(Address x, Address y);
			friend inline bool operator>=(Address x, Address y);
			friend inline Address operator&(Address x, Address y);
			template<class charT, class Traits> friend std::basic_ostream<charT, Traits>& operator<<(std::basic_ostream<charT, Traits>& os, const Address& address);
			template<class charT, class Traits> friend std::basic_istream<charT, Traits>& operator>>(std::basic_istream<charT, Traits>& is, Address& address);
			//! Data representation of the IPv4 address
			uint32_t data;
		};

		//! Compare two Address values
		/*!
		 * This comparator merely passes on the comparison to the internal 
		 * unsigned 32 bit integer.
		 */
		inline bool operator==(Address x, Address y) { return x.data==y.data; }
		//! Compare two Address values
		/*!
		 * This comparator merely passes on the comparison to the internal 
		 * unsigned 32 bit integer.
		 */
		inline bool operator>(Address x, Address y) { return x.data>y.data; }
		//! Compare two Address values
		/*!
		 * This comparator merely passes on the comparison to the internal 
		 * unsigned 32 bit integer.
		 */
		inline bool operator<(Address x, Address y) { return x.data<y.data; }
		//! Compare two Address values
		/*!
		 * This comparator merely passes on the comparison to the internal 
		 * unsigned 32 bit integer.
		 */
		inline bool operator<=(Address x, Address y) { return x.data<=y.data; }
		//! Compare two Address values
		/*!
		 * This comparator merely passes on the comparison to the internal 
		 * unsigned 32 bit integer.
		 */
		inline bool operator>=(Address x, Address y) { return x.data>=y.data; }
		//! Bitwise AND two Address values
		/*!
		 * The bitwise AND operation is passed on to the internal unsigned 32 bit integer
		 */
		inline Address operator&(Address x, Address y) { return Address(x.data&y.data); }

		//! Address stream insertion operation
		/*!
		 * This stream inserter obeys all stream manipulators regarding alignment, field width and numerical base.
		 */
		template<class charT, class Traits> std::basic_ostream<charT, Traits>& operator<<(std::basic_ostream<charT, Traits>& os, const Address& address);
		//! Address stream extractor operation
		/*!
		 * In order for this to work the stream must be positioned on at the start of a standard decimal representation
		 * of a IPv4 address. Example: "127.0.0.1".
		 */
		template<class charT, class Traits> std::basic_istream<charT, Traits>& operator>>(std::basic_istream<charT, Traits>& is, Address& address);

		//! Data structure of HTTP environment data
		/*!
		 * This structure contains all HTTP environment data for each individual request. The data is processed
		 * from FastCGI parameter records.
		 *
		 * @tparam charT Character type to use for strings
		 */
		template<class charT>
		struct Environment
		{
			//! Hostname of the server
			std::basic_string<charT> host;
			//! User agent string
			std::basic_string<charT> userAgent;
			//! Content types the client accepts
			std::basic_string<charT> acceptContentTypes;
			//! Languages the client accepts
			std::basic_string<charT> acceptLanguages;
			//! Character sets the clients accepts
			std::basic_string<charT> acceptCharsets;
			//! Referral URL. Percent symbol escaped bytes are converted to their actual value.
			std::basic_string<charT> referer;
			//! Content type of data sent from client
			std::basic_string<charT> contentType;
			//! Query string appended to the URL submitted by the client. Percent symbol escaped bytes are converted to their actual value.
			std::basic_string<charT> queryString;
			//! Cookie string sent from the client
			std::basic_string<charT> cookies;
			//! HTTP root directory
			std::basic_string<charT> root;
			//! Filename of script relative to the HTTP root directory
			std::basic_string<charT> scriptName;
			//! The etag the client assumes this document should have
			int etag;
			//! How many seconds the connection should be kept alive
			int keepAlive;
			//! Length of content to be received from the client (post data)
			unsigned int contentLength;
			//! IP address of the server
			Address serverAddress;
			//! IP address of the client
			Address remoteAddress;
			//! TCP port used by the server
			uint16_t serverPort;
			//! TCP port used by the client
			uint16_t remotePort;
			//! Timestamp the client has for this document
			boost::posix_time::ptime ifModifiedSince;

			//typedef std::map<std::basic_string<charT>, std::basic_string<charT> > OtherData;
			//OtherData otherData;

			typedef std::map<std::basic_string<charT>, Post<charT> > Posts;
			//! STL container associating Post objects with their name
			Posts posts;

			//! Parses FastCGI parameter data into the data structure
			/*!
			 * This function will take the body of a FastCGI parameter record and parse
			 * the data into the data structure. data should equal the first character of
			 * the records body with size being it's content length.
			 *
			 * @param[in] data Pointer to the first byte of parameter data
			 * @param[in] size Size of data in bytes
			 */
			bool fill(const char* data, size_t size);
			//! Parses raw http post data into the posts object
			/*!
			 * This function will take arbitrarily divided chunks of raw http post
			 * data and parse them into the posts container of Post objects. data should
			 * equal the first bytes of the FastCGI IN records body with size being it's
			 * content length.
			 *
			 * @param[in] data Pointer to the first byte of post data
			 * @param[in] size Size of data in bytes
			 */
			void fillPosts(const char* data, size_t size);

			//! Clear the post buffer
			void clearPostBuffer() { postBuffer.reset(); postBufferSize=0; }

		private:
			//! Raw string of characters representing the post boundary
			boost::scoped_array<char> boundary;
			//! Size of boundary
			size_t boundarySize;

			//! Buffer for processing post data
			boost::scoped_array<char> postBuffer;
			//! Size of data in postBuffer
			size_t postBufferSize;
		};

		//! Convert a char string to a std::wstring
		/*!
		 * @param[in] data First byte in char string
		 * @param[in] size Size in bytes of the string (no null terminator)
		 * @param[out] string Reference to the wstring that should be modified
		 * @return Returns true on success, false on failure
		 */
		bool charToString(const char* data, size_t size, std::wstring& string);
		//! Convert a char string to a std::string
		/*!
		 * @param[in] data First byte in char string
		 * @param[in] size Size in bytes of the string (no null terminator)
		 * @param[out] string Reference to the string that should be modified
		 * @return Returns true on success, false on failure
		 */
		inline bool charToString(const char* data, size_t size, std::string& string) { string.assign(data, size); return true; }
		//! Convert a char string to an integer
		/*!
		 * This function is very similar to std::atoi() except that it takes start/end values
		 * of a non null terminated char string instead of a null terminated string. The first
		 * character must be either a number or a minus sign (-). As soon as the end is reached
		 * or a non numerical character is reached, the result is tallied and returned.
		 *
		 * @param[in] start Pointer to the first byte in the string
		 * @param[in] end Pointer to the last byte in the string + 1
		 * @return Integer value represented by the string
		 */
		int atoi(const char* start, const char* end);

		//! Finds the value associated with a name in an 'name="value"' string
		/*!
		 * Note that the quotation marks are removed from the value. If no value is found, 
		 * then string is left unchanged.
		 *
		 * @param[in] name Pointer to a null terminated string containing the name
		 * @param[in] start Pointer to the first byte of data to look in
		 * @param[in] end Pointer to the last byte of data to look in + 1
		 * @param[out] string Reference to the string the value should be stored in.
		 *
		 * @return Returns false if the name isn't found. True otherwise.
		 */
		template<class charT>
		bool parseXmlValue(const char* const name, const char* start, const char* end, std::basic_string<charT>& string);

		/** 
		 * @brief Finds the value associated with a name in a 'name=value&name2=value2' string
		 * 
		 * Use this function to find values in the name/value pairs in get/cookie strings.
		 *
		 * @param[in] name Name of field
		 * @param[in] data Data to look for it in
		 * @param[out] value String to store the value in
		 * @param[in] fieldSep Character that separates fields (default: '&')
		 *
		 * @return Returns false if the name isn't found. True otherwise.
		 */
		template<class charT>
		bool parseValue(const std::basic_string<charT>& name, const std::basic_string<charT>& data, std::basic_string<charT>& value, charT fieldSep='&');

		//! Convert a string with percent escaped byte values to their actual values
		/*!
		 *	Since converting a percent escaped string to actual values can only make it shorter, 
		 *	it is safe to assume that the return value will always be smaller than size. It is
		 *	thereby a safe move to make the destination block of memory the same size as the source.
		 *
		 * @param[in] source Pointer to the first character in the percent escaped string
		 * @param[in] size Size in bytes of the data pointed to by source (no null termination)
		 * @param[out] destination Pointer to the section of memory to write the converted string to
		 * @return Actual size of the new string
		 */
		int percentEscapedToRealBytes(const char* source, char* destination, size_t size);

		/** 
		 * @brief List of characters in order for Base64 encoding.
		 */
		extern const char base64Characters[];

		/** 
		 * @brief Convert a binary container of data to a Base64 encoded container.
		 *
		 * If destination is a fixed size container, it should have a size of at least ((end-start-1)/3 + 1)*4 not including null terminators if used and assuming integer arithmetic.
		 *
		 * @param[in] start Iterator to start of binary data.
		 * @param[in] end Iterator to end of binary data.
		 * @param[out] destination Iterator to start of Base64 destination.
		 *
		 * @tparam In Input iterator type. Should be dereferenced to type char.
		 * @tparam Out Output iterator type. Should be dereferenced to type char.
		 */
		template<class In, class Out> void base64Encode(In start, In end, Out destination);

		/** 
		 * @brief Convert a Base64 encoded container to a binary container.
		 *
		 * If destination is a fixed size container, it should have a size of at least (end-start)*3/4 not including null terminators if used.
		 * 
		 * @param[in] start Iterator to start of Base64 data.
		 * @param[in] end Iterator to end of Base64 data.
		 * @param[out] destination Iterator to start of binary destination.
		 *
		 * @tparam In Input iterator type. Should be dereferenced to type char.
		 * @tparam Out Output iterator type. Should be dereferenced to type char.
		 * 
		 * @return Iterator to last position in destination
		 */
		template<class In, class Out> Out base64Decode(In start, In end, Out destination);
	}
}

template<class In, class Out> Out Fastcgipp::Http::base64Decode(In start, In end, Out destination)
{
	for(int buffer, bitPos=-8, padStart; start!=end || bitPos>-6; ++destination)
	{
		if(bitPos==-8)
		{
			bitPos=18;
			padStart=-6;
			buffer=0;
			while(bitPos!=-6)
			{
				if(start==end) throw;
				int value=*start++;
				if(value >= 'A' && 'Z' >= value) value -= 'A';
				else if(value >= 'a' && 'z' >= value) value -= 'a' - 26;
				else if(value >= '0' && '9' >= value) value -= '0' - 52;
				else if(value == '+') value = 62;
				else if(value == '/') value = 63;
				else if(value == '=') { padStart+=8; value=0; }
				else throw;

				buffer |= value << bitPos;
				bitPos-=6;
			}
			bitPos=16;
		}

		if(padStart==bitPos)
			return ++destination;
		*destination++ = (buffer >> bitPos) & 0xff;
		bitPos-=8;
	}

	return destination;
}

template<class In, class Out> void Fastcgipp::Http::base64Encode(In start, In end, Out destination)
{
	for(int buffer, bitPos=-6, padded; start!=end || bitPos>-6; ++destination)
	{
		if(bitPos==-6)
		{
			bitPos=16;
			buffer=0;
			padded=-6;
			while(bitPos!=-8)
			{
				if(start!=end) 
					buffer |= (uint32_t)*start++ << bitPos;
				else padded+=6;
				bitPos-=8;
			}
			bitPos=18;
		}

		if(padded == bitPos)
		{
			*destination='=';
			padded-=6;
		}
		else *destination=base64Characters[ (buffer >> bitPos)&0x3f ];
		bitPos -= 6;
	}
}

#endif