/*
 * Copyright (c) 2013-2017, Collobos Software Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef _nodeoze_bitmap_h
#define _nodeoze_bitmap_h	

#include <cstdint>

#if defined( max )
#	undef max
#endif

namespace nodeoze {

class bitmap
{
public:
		
	static const std::size_t bitword_size = 64;
	using bitword = std::uint64_t;
		
	/*! \brief Null constructor
			
			Creates an empty bit vector, corresponding to an empty set.
		*/
	bitmap()
	:
		m_words()
	{
	}
		
	/*! \brief Construct with specified capacity
		\param num_words the number of unsigned long long words to pre-allocate
			
		Constructs an instance whose underlying vector is initialized with the
		indicated number of zero-filled words.
	*/

	bitmap( std::size_t num_words )
	:
		m_words( num_words, 0ULL )
	{
	}
		
	/*! \brief Construct with specified capacity and set the specified bit
		\param num_words the number of unsigned long long words to pre-allocate
			
		Constructs an instance whose underlying vector is initialized with the
		indicated number of zero-filled words, then sets the bit at \c bit_position.
	*/

	bitmap( std::size_t num_words, std::size_t bit_position )
	:
		m_words( ( num_words > ( ( bit_position / bitword_size) + 1)) ? num_words: ( ( bit_position / bitword_size) + 1), 0ULL )
	{
		set_bit( bit_position);
	}
		
	/*! \brief Construct from word vector (copy)
		\param words the vector whose values will be copied into the underlying vector of
		the created instance.
			
		Constructs an instance, copying the values in the \c words parameter to
		the underlying vector of the created instance.
	*/

	bitmap( const std::vector< bitword >& words )
	:
		m_words( words )
	{
	}
		
	/*! \brief Construct from word vector (move)
		\param words the vector whose contents will be moved into the underlying vector of
		the created instance
			
		Constructs an instance, moving the contents of the \c words parameter into
		the underlying vector of the created instance.
	*/

	bitmap( std::vector< bitword >&& words )
	:
		m_words( std::move( words ) )
	{
	}
		
 	/*! \brief Copy constructor
		\param rhs the instance whose contents will be copied
			
		Constructs an instance, copying the contents of the \c rhs parameter
		into the created instance.
	*/

	bitmap( const bitmap& rhs )
	:
		m_words( rhs.m_words )
	{
	}
		
	/*! \brief Move constructor
		\param rhs the instance whose contents will be moved
			
		Constructs an instance, moving the contents of the \c rhs parameter
		into the created instance.
	*/

	bitmap( bitmap&& rhs )
	:
		m_words( std::move( rhs.m_words ) )
	{
	}
		
	/*! \brief Copy assignment operator
		\param rhs the instance whose value will be assigned to this instance
		\return a reference to this instance
			
		Copies the contents of the \c rhs parameter into the underlying
		vector of this instance.
	*/

	bitmap&
	operator=( const bitmap& rhs )
	{
		m_words = rhs.m_words;
		return *this;
	}
		
	/*! \brief Move assignment operator
		\param rhs the instance whose value will be assigned to this instance
		\return a reference to this instance
			
		Moves the contents of the \c rhs parameter into the underlying
		vector of this instance.
	*/

	bitmap&
	operator=( bitmap&& rhs )
	{
		m_words = std::move( rhs.m_words );
		return *this;
	}
		
	/*! \brief Empty predicate
	 	\return true if all bits are zero
			
		Returns true if all values in the underlying vector are zero,
		which corresponds to the represented set having the value 
		\f$ \emptyset \f$.
	*/

	bool
	empty() const
	{
		return hamming_weight() == 0;
	}
		
	/*! \brief Clear all bits
			
		Assigns the value zero to all elements of the underlying
		vector. This corresponds to assigning the value 
		\f$ \emptyset \f$ to the represented set.
	*/

	void
	clear()
	{
		for ( auto i = 0u; i < m_words.size(); ++i )
		{
			m_words[ i ] = 0ULL;
		}
	}
		
	/*! \brief Hamming weight
		\return the Hamming weight of the underlying vector
			
		Returns the Hamming weight -- the number of bits set to one 
		-- of the underlying vector. This corresponds to the
		cardinality of the represented set.
	*/

	size_t
	hamming_weight() const
	{
		std::size_t bcount = 0u;

		for ( auto it = m_words.begin(); it != m_words.end(); ++it )
		{
			bcount += count_bits_set( *it );
		}

		return bcount;
	}
		
	/*! \brief Test a specified bit
		\param bit_position position of the bit to be tested
			
		If the bit specified by the \c bit_position parameter
		in the underlying bit vector is one, return \c true.
		Otherwise, return \c false.
	*/

	bool
	is_bit_set( std::size_t bit_position ) const
	{
		auto word_offset = bit_position / bitword_size;

		if ( m_words.size() < word_offset + 1 )
		{
			return false;
		}
		else
		{
			auto bit_offset = bit_position % bitword_size;
			return ( m_words[ word_offset ] & ( 1ULL << bit_offset) ) != 0ULL;
		}
	}
		
	/*! \brief Set a specified bit to one
		\param bit_position position of the bit to be set
			
		Sets the bit specified by the \c bit_position parameter
		in the underlying bit vector to one. The corresponding
		element is added to the represented set, if absent.
	*/

	bitmap&
	set_bit( std::size_t bit_position )
	{
		auto word_offset = bit_position / bitword_size;

		for ( auto i = m_words.size(); i < word_offset + 1; ++i )
		{
			m_words.push_back( 0ULL );
		}

		auto bit_offset = bit_position % bitword_size;
		m_words[ word_offset ] |= ( 1ULL << bit_offset);
		return *this;
	}
		
	/*! \brief Clear a specified bit
		\param bit_position position of the bit to be cleared
			
		Clears the bit specified by the \c bit_position parameter
		in the underlying bit vector. The corresponding
		element is removed from the represented set, if present.
	*/

	bitmap&
	clear_bit( std::size_t bit_position )
	{
		auto word_offset = bit_position / bitword_size;

		for ( auto i = m_words.size(); i < word_offset + 1; ++i )
		{
			m_words.push_back( 0ULL );
		}

		auto bit_offset = bit_position % bitword_size;
		m_words[ word_offset ] &= ( ~( 1ULL << bit_offset));
		return *this;
	}

	/*! \brief Equality comparator
		\param rhs bit vector to compare with this instance
		\return true if the compared value is equal to this instance, false otherwise
			
		Returns true if the parameter bit vector and this
		instance are equal. Vectors are considered equal 
		if all bits set in this instance are also set 
		in the \c rhs parameter vector, and
		the hamming weight of this instance is equal to the
		hamming weight of \c rhs. This corresponds to 
		comparison for equality of the represented sets.
	*/
	
	bool
	operator==( const bitmap& rhs) const
	{
		if ( m_words.size() > rhs.m_words.size())
		{
			for ( auto i = 0u; i < m_words.size(); ++i )
			{
				if ( i < rhs.m_words.size() )
				{
					if ( m_words[ i ] !=  rhs.m_words[ i ] )
					{
						return false;
					}
				}
				else
				{
					if ( m_words[ i ] != 0ULL )
					{
						return false;
					}
				}
			}
			return true;
		}
		else // m_words.size() <= rhs.m_words.size()
		{
			for ( auto i = 0u; i < rhs.m_words.size(); ++i )
			{
				if ( i < m_words.size() )
				{
					if ( m_words[ i ] !=  rhs.m_words[ i ] )
					{
						return false;
					}
				}
				else
				{
					if ( rhs.m_words[ i ] != 0ULL )
					{
						return false;
					}
				}
			}
			return true;
		}
	}
		
	/*! \brief Inequality comparator
		\param rhs bit vector to compare with this instance
		\return true if the compared value is not equal to this instance, false otherwise
			
		Returns true if the parameter bit vector and this
		instance are not equal. Vectors are considered equal
		if all bits set in this instance are also set 
		in the \c rhs parameter vector, and
		the hamming weight of this instance is equal to the
		hamming weight of \c rhs. This corresponds to 
		comparison for inequality of the represented sets.
	*/
	
	bool
	operator!=( const bitmap& rhs) const
	{
		return ! this->operator==( rhs);
	}
		
	/*! \brief Bitwise OR operator
		\param rhs bit vector to be combined with this instance
		\return the result of the OR operation
			
		Returns a newly-constructed instance whose value is the bitwise
		OR of this instance and the \c rhs parameter, corresponding
		to the union of the two represented sets.
	*/
	
	bitmap
	operator|( const bitmap& rhs ) const
	{
		auto usize = std::max( m_words.size(), rhs.m_words.size() );
		std::vector< bitword > uwords( usize, 0ULL );

		for ( auto i = 0u; i < usize; ++i )
		{
			uwords[ i ] = ( ( i < m_words.size()) ? m_words[ i ] : 0ULL ) | ( ( i < rhs.m_words.size() ) ? rhs.m_words[ i ] : 0ULL );
		}

		return bitmap( std::move( uwords ) );	
	}
		
	/*! \brief Bitwise AND operator
		\param rhs bit vector to be combined with this instance
		\return the result of the AND operation
			
		Returns a newly constructed instance whose value is the bitwise
		AND of this instance and the \c rhs parameter, corresponding
		to the intersection of the two represented sets.
	*/

	bitmap
	operator&( const bitmap& rhs ) const
	{
		auto result_size = std::max( m_words.size(), rhs.m_words.size() );
		std::vector< bitword > result_words( result_size, 0ULL );

		for ( auto i = 0u; i < result_size; ++i )
		{
			result_words[ i ] = ( ( i < m_words.size()) ? m_words[ i ] : 0ULL ) & ( ( i < rhs.m_words.size() ) ? rhs.m_words[ i ] : 0ULL );
		}

		return bitmap( std::move( result_words ) );
	}
		
	/*! \brief Bitwise subtraction operator
		\param rhs bit vector to be subtracted from this instance
		\return the result of the subtraction operation
			
		Returns a newly constructed instance whose value 
		is the result of clearing each bit in this instance that
		corresponds to a bit in \c rhs with the value one,
		equivalent to <code> ( *this & ( ~rhs ) ) </code>.
		This corresponds to the set difference of the
		sets represented by this instance and \c rhs.
	*/

	bitmap
	operator-( const bitmap& rhs ) const
	{
		auto result_size = std::max( m_words.size(), rhs.m_words.size() );
		std::vector< bitword > result_words( result_size, 0ULL );

		for ( auto i = 0u; i < result_size; ++i )
		{
			result_words[ i ] = ( ( i < m_words.size()) ? m_words[ i ] : 0ULL ) & ( ~ ( ( i < rhs.m_words.size() ) ? rhs.m_words[ i ] : 0ULL ) );
		}

		return bitmap( std::move( result_words ) );
	}
		
	/*! \brief Bitwise OR assignment operator
		\param rhs bit vector to be combined with this instance
		\return a reference to this instance
			
		Assigns the result of a bitwise OR of this instance with
		the \c rhs parameter to this instance, equivalent
		to <code> *this = *this | rhs </code>.
	*/

	bitmap&
	operator|=( const bitmap& rhs )
	{
		for ( auto i = 0ULL; i < rhs.m_words.size(); ++i )
		{
			if ( i < m_words.size() )
			{
				m_words[ i ] |= rhs.m_words[ i ];
			}
			else
			{
				m_words.push_back( rhs.m_words[ i ] );
			}
		}

		return *this;
	}
		
	/*! \brief Bitwise AND assignment operator
		\param rhs bit vector to be combined with this instance
		\return a reference to this instance
			
		Assigns the result of a bitwise AND of this instance with
		the \c rhs parameter to this instance, equivalent
		to <code> *this = *this & rhs </code>. 
		If \f$ A \f$ is the set represented by this
		instance and \f$ B \f$ is the set represented by \c rhs, 
		then the result of this operation is isomorphic to the
		relative complement of B with respect to A,
		\f$ A \subseteq B \f$.

	*/
	
	bitmap&
	operator&=( const bitmap& rhs )
	{
		for ( auto i = 0ULL; i < rhs.m_words.size(); ++i )
		{
			if ( i < m_words.size() )
			{
				m_words[ i ] &= rhs.m_words[ i ];
			}
			else
			{
				m_words.push_back( rhs.m_words[ i ] );
			}
		}

		return *this;
	}
		
	/*! \brief Bit subtraction assignment operator
		\param rhs bit vector to be subtracted from this instance
		\return a reference to this instance
			
		Assigns the result of bitwise subtraction of \c rhs from 
		this instance to this instance, equivalent to 
		<code> *this = *this & ( ~ rhs ) </code>.
	*/

	bitmap&
	operator-=( const bitmap& rhs )
	{
		for ( auto i = 0u; i < rhs.m_words.size(); ++i )
		{
			if ( i >= m_words.size() )
			{
				break;
			}

			m_words[ i ] &= ( ~ ( rhs.m_words[ i ] ) );
		}

		return *this;
	}
		
	/*! \brief Contained comparator
		\param rhs bit vector to be compared with this instance
		\return true if all bits set in this instance are also set in \c rhs
		
		This operation returns true if, for each bit position in this
		instance with a value of 1, the same position in \c rhs also
		has a value of 1, equivalent to 
		<code> ( *this & rhs ) == *this  </code>.
		If \f$ A \f$ is the set represented by this
		instance and \f$ B \f$ is the set represented by \c rhs, 
		then this operation is isomorphic to the subset predicate,
		\f$ A \setminus B \f$.
	*/
	bool
	operator<=( const bitmap& rhs ) const
	{
		for ( auto i = 0u; i < m_words.size(); ++i )
		{
			if ( m_words[ i ] != ( m_words[ i ] & ( ( i < rhs.m_words.size() ) ? rhs.m_words[ i ] : 0ULL )))
			{
				return false;
			}
		}

		return true;
	}
		
	/*! \brief Contains comparator
		\param rhs bit vector to be compared with this instance
		\return true if all bits set \c rhs are also set in in this instance
		
		This operation returns true if, for each bit position in
		\c rhs with a value of 1, the same position in this instance
		also has a value of 1, equivalent to
		<code> ( *this & rhs ) == rhs  </code>, or <code> rhs <= *this </code>.
		If \f$ A \f$ is the set represented by this
		instance and \f$ B \f$ is the set represented by \c rhs, 
		then this operation is isomorphic to the superset predicate,
		\f$ A \supseteq B \f$.
	*/
	bool
	operator>=( const bitmap& rhs ) const
	{
		return rhs <= ( *this);
	}
		
	const std::vector< bitword >&
	get_words() const
	{
		return m_words;
	}
		
	/*! \brief Disjoint comparator
		\param rhs bit vector to be compared with this instance
		\return true if no bit set \c rhs is also set in in this instance
		
		This operation returns true if this instance and \c rhs 
		are disjoint, that is, they have no set bit positions 
		in common, equivalent to <code> ( *this & rhs ).empty() </code>.
		If this is true, then the represented sets are disjoint.
	*/
	static bool
	disjoint( const bitmap& lhs, const bitmap& rhs )
	{
		auto max_size = std::max( lhs.m_words.size(), rhs.m_words.size() );

		for ( auto i = 0u; i < max_size; ++i )
		{
			if
			(
				(
					( ( i < lhs.m_words.size() ) ? lhs.m_words[ i ] : 0ULL ) &
					( ( i < rhs.m_words.size() ) ? rhs.m_words[ i ] : 0ULL )
				) != 0ULL
			)
			{
				return false;
			}
		}

		return true;
	}
		
	/*! \brief Empty predicate (static)
		\param arg subject of predicate
		\return true if all bits in arg are zero
		
		Returns true if all values in the underlying vector
		of \c arg are zero, which corresponds to the set
		represented by \c arg having the value
		\f$ \emptyset \f$. Equivalent to 
		<code> arg.empty() </code>.
	*/

	static bool
	empty( const bitmap& arg )
	{
		return arg.empty();
	}

protected:
	
	static
	size_t count_bits_set( bitword bset)
	{
		/*
		* A.K.A. Hamming weight.
		*
		* This is write-only code, revealed by the gods. Probably best not
		* to mess with it. If you feel the need, replace it with a shift loop.
		* Don't let the visual complexity fool you -- it's much faster than 
		* loop-shifting. Basically 12 operations, all of them bitwise, shift, 
		* or add/subtract except for one multiply. Assuming your CPU has a
		* hardware barrel-shifter, of course. Pretty much every garden-variety
		* CPU since 1986 has had one, except for the Pentium 4, the performance
		* of which sucked notoriously, largely for lack of a hardware barrel
		* shifter. Modern crypto algorithms demand it; they tend to be shifty.
		*/
		bset = bset - ( ( bset >> 1 ) & 0x5555555555555555ULL );
		bset = ( bset & 0x3333333333333333ULL ) + ( ( bset >> 2 ) & 0x3333333333333333ULL );
		return ( std::size_t ) ( ( ( ( bset + ( bset >> 4 ) ) & 0x0F0F0F0F0F0F0F0FULL ) * 0x0101010101010101ULL ) >> 56 );
	}

	static bool
	power_of_2( bitword n )
	{
		return ( n != 0 ) && ( ( n & ( n - 1 ) ) == 0 );
	}

	static int
	bit_position( bitword n )
	{
		if ( ! power_of_2( n ) )
		{
			return -1;
		}

		auto count = 0;

		while ( n != 0 )
		{
			n >>= 1;
			++count;
		}

		return count;
	}
	
	std::vector< bitword > m_words;
};

}

#endif
