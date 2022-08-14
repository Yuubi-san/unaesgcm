
#ifndef UNAESGCM_HPP
#define UNAESGCM_HPP

#include "hex.hpp"
#include <vector>
#include <array>
#include <variant>
#include <utility>
#include <limits>
#include <iosfwd>

template<std::size_t N>
struct bits_to_bytes
{
  static_assert( N % byte_bits == 0 );
  static constexpr auto value = N / byte_bits;
};
template<std::size_t N>
constexpr auto bits = bits_to_bytes<N>::value;

constexpr auto tag_bits = 128u;

template<auto key_bits = 256u>
inline auto parse_iv_and_key( const std::string_view iv_and_key )
{
  constexpr auto key_bytes  = bits<key_bits>;
  constexpr auto key_digits = key_bits/digit_bits;

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

using aes_key = std::variant
<
  std::array<byte,bits<256>>,
  std::array<byte,bits<192>>,
  std::array<byte,bits<128>>
>;

template<typename... T>
constexpr auto data( const std::variant<T...> &v )
{
  using std::data;
  return std::visit( [](const auto &cont){return data(cont);}, v );
}

template<typename... T>
constexpr auto size( const std::variant<T...> &v )
{
  using std::size;
  return std::visit( [](const auto &cont){return size(cont);}, v );
}

void aesgcm(
  const std::vector<byte> &iv, const aes_key &key,
  std::istream &in, std::ostream &out );

[[nodiscard]]
bool unaesgcm(
  const std::vector<byte> &iv, const aes_key &key,
  std::istream &in, std::ostream &out );

#endif
