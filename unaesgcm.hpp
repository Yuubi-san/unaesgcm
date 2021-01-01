
#ifndef UNAESGCM_HPP
#define UNAESGCM_HPP

#include "hex.hpp"
#include <vector>
#include <array>
#include <utility>
#include <limits>
#include <iosfwd>

constexpr auto key_bits   = 256u;
static_assert( key_bits % byte_bits == 0 );
constexpr auto key_bytes  = key_bits/byte_bits;
constexpr auto key_digits = key_bits/digit_bits;
constexpr auto tag_bits   = 128u;

inline auto parse_iv_and_key( const std::string_view iv_and_key )
{
  const auto iv_and_key_digits = std::size(iv_and_key);
  if ( iv_and_key_digits < key_digits )
    throw std::runtime_error{
      "key too short ("+ std::to_string(iv_and_key_digits) +" chars)"};

  const auto iv_digits = iv_and_key_digits - key_digits;
  if ( iv_digits % byte_digits )
    throw std::runtime_error{
      "non-whole-byte IV size ("+ std::to_string(iv_digits) +" chars)"};

  const auto iv  = parse_hex( std::data(iv_and_key),
    std::vector<byte>(iv_digits/byte_digits), "IV" );
  const auto key = parse_hex( std::data(iv_and_key)+iv_digits,
    std::array<byte,key_bytes>{}, "key" );
  return std::pair{iv,key};
}

bool unaesgcm(
  const std::vector<byte> &iv, const std::array<byte,key_bytes> &key,
  std::istream &in, std::ostream &out );

#endif
