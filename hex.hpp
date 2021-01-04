
#ifndef UNAESGCM_HEX_HPP
#define UNAESGCM_HEX_HPP

#include <charconv>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#include <limits>
#include <iomanip>

using byte = unsigned char;
constexpr auto byte_bits = std::numeric_limits<byte>::digits;

constexpr auto digit_bits  = 4u;
static_assert( byte_bits % digit_bits == 0 );
constexpr auto byte_digits = byte_bits/digit_bits;

inline std::optional<byte> hex_to_byte( const char *const hex ) noexcept
{
  const auto end = hex + byte_digits;
  byte res{};
  if ( std::from_chars(hex, end, res, 16).ptr == end )
    return res;
  else
    return {};
}

template<typename Cont>
inline auto parse_hex(
  const char *hex, Cont &&bytes, const std::string_view desc )
{
  for ( auto &b : bytes )
  {
    if ( const auto maybe = hex_to_byte(hex); maybe )
      b = *maybe;
    else
      throw std::runtime_error{
        "bad hex "+ std::string{desc} +" byte: "+
        std::string{hex,hex+byte_digits} };
    hex += byte_digits;
  }
  return std::forward<Cont>(bytes);
}

template<typename Stream, typename Cont>
auto &dump_hex( Stream &out, const Cont &octets )
{
  const auto oldflags = out.flags();
  out.setf(std::ios_base::hex, std::ios_base::basefield);
  const auto oldfill  = out.fill('0');
  for ( const auto o : octets )
  {
    out.width(byte_digits);
    out << unsigned{o} << ' ';
  }
  out << '\b';
  out.fill (oldfill);
  out.flags(oldflags);
  return out;
}

#endif
