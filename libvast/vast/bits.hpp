#ifndef VAST_BITS_HPP
#define VAST_BITS_HPP

#include "vast/word.hpp"

namespace vast {

/// A sequence of bits represented by a single word. If the size is greater
/// than or equal to the word size, then the data block must be all 0s or
/// all 1s. Otherwise, only the N least-significant bits are active, and the
/// remaining bits in the block are guaranteed to be 0.
template <class T>
class bits {
  public:
  using word_type = word<T>;
  using value_type = typename word_type::value_type;
  using size_type = uint64_t;

  static constexpr value_type mask(value_type x, size_type n) {
    return n < word_type::width ? x & word_type::lsb_mask(n) : x;
  }

  /// Constructs a bit sequence.
  /// @param x The value of the sequence.
  /// @param n The length of the sequence.
  /// @pre `n > 0 && (n < w || all_or_none(x))` where *w* is the word width.
  bits(value_type x = 0, size_type n = word_type::width)
    : data_{mask(x, n)},
      size_{n} {
    VAST_ASSERT(n > 0);
    VAST_ASSERT(n <= word_type::width || word_type::all_or_none(x));
  }

  value_type data() const {
    return data_;
  }

  size_type size() const {
    return size_;
  }

  /// Accesses the i-th bit in the bit sequence.
  /// @param i The bit position counting from the LSB.
  /// @returns The i-th bit value from the LSB.
  /// @pre `i < size`.
  bool operator[](size_type i) const {
    VAST_ASSERT(i < size_);
    return i > word_type::width ? !!data_ : data_ & word_type::mask(i);
  }

  /// Checks whether all bits have the same value.
  /// @returns `true` if the bits are either all 0 or all 1.
  bool homogeneous() const {
    return size_ == word_type::width 
      ? word_type::all_or_none(data_) 
      : word_type::all_or_none(data_, size_);
  }

  /// Computes the number 1-bits.
  /// @returns The population count of this bit sequence.
  size_type count() const {
    if (size_ <= word_type::width && data_ > 0)
      return word_type::popcount(data_);
    return data_ == word_type::all ? size_ : 0;
  }

  /// Finds the first bit of a particular value.
  /// @tparam Bit The bit value to look for.
  /// @returns The position of the first bit having value *Bit*, or `npos` if
  ///          not such position exists..
  template <bool Bit = true>
  size_type find_first() const {
    auto data = Bit ? data_ : ~data_;
    if (size_ > word_type::width)
      return data == word_type::all ? 0 : word_type::npos;
    if (data == word_type::none)
      return word_type::npos;
    return word_type::count_trailing_zeros(data);
  }

  /// Finds the next bit after at a particular offset.
  /// @param i The offset after where to begin searching.
  /// @returns The position *p*, where *p > i*, of the bit having value *Bit*,
  ///          or `npos` if no such *p* exists.
  template <bool Bit = true>
  size_type find_next(size_type i) const {
    if (i >= size_ - 1)
      return word_type::npos;
    auto data = Bit ? data_ : ~data_;
    if (size_ > word_type::width)
      return data == word_type::all ? i + 1 : word_type::npos;
    data &= ~word_type::lsb_mask(i + 1);
    if (data == word_type::none)
      return word_type::npos;
    return word_type::count_trailing_zeros(data);
  }

  /// Finds the last bit of a particular value.
  /// @tparam Bit The bit value to look for.
  /// @returns The position of the last bit having value *Bit*.
  template <bool Bit = true>
  size_type find_last() const {
    auto data = Bit ? data_ : ~data_;
    if (size_ > word_type::width)
      return data == word_type::all ? size_ - 1 : word_type::npos;
    if (data == word_type::none)
      return word_type::npos;
    return word_type::width - word_type::count_leading_zeros(data) - 1;
  }

private:
  value_type data_;
  size_type size_;
};

// -- algorithms -------------------------------------------------------------

/// Computes the number of occurrences of a bit value in *[0,i]*.
/// @tparam Bit The bit value to count.
/// @param b The bit sequence to count.
/// @param i The offset where to end counting.
/// @returns The population count of *b* up to and including position *i*.
/// @pre `i < b.size()`
template <bool Bit = true, class T>
typename bits<T>::size_type
rank(const bits<T>& b, typename bits<T>::size_type i) {
  using word_type = typename bits<T>::word_type;
  VAST_ASSERT(i < b.size());
  auto data = Bit ? b.data() : ~b.data();
  if (b.size() > word_type::width)
    return data == word_type::none ? 0 : i + 1;
  if (i == word_type::width - 1)
    return data == word_type::none ? 0 : word_type::popcount(data);
  return word_type::rank(data, i);
}

/// Computes the number of occurrences of a bit value.
/// @tparam Bit The bit value to count.
/// @param b The bit sequence to count.
/// @returns The population count of *b*.
template <bool Bit = true, class T>
auto rank(const bits<T>& b) {
  return rank<Bit>(b, b.size() - 1);
}

/// Computes the position of the i-th occurrence of a bit.
/// @tparam Bit the bit value to locate.
/// @param b The bit sequence to select from.
/// @param i The position of the *i*-th occurrence of *Bit* in *b*.
/// @pre `i > 0 && i <= b.size()`
template <bool Bit = true, class T>
typename bits<T>::size_type
select(const bits<T>& b, typename bits<T>::size_type i) {
  using word_type = typename bits<T>::word_type;
  VAST_ASSERT(i > 0);
  VAST_ASSERT(i <= b.size());
  auto data = Bit ? b.data() : ~b.data();
  if (b.size() > word_type::width)
    return data == word_type::all ? i - 1 : word_type::npos;
  return word_type::select(data, i);
}

} // namespace vast

#endif
