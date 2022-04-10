
#include "unaesgcm.hpp"
#include "hex.hpp"
#include "fixcapvec.hpp"
#include <sstream>
#include <cassert>

template<
  template<typename, std::size_t> class Cont,
  char Oh, char Eks, char... Digits>
constexpr auto parse_hex()
{
  using std::data; using std::size;
  static_assert( Oh == '0' and (Eks == 'x' or Eks == 'X') );
  constexpr std::array digits{Digits...};
  constexpr auto total_bits = std::size(digits) * digit_bits;
  static_assert( total_bits % byte_bits == 0 );
  Cont<byte, total_bits/byte_bits> bytes;
  bytes.resize( total_bits/byte_bits );
  return parse_hex( std::data(digits), std::move(bytes), "literal" );
}

template<char... Digits> inline auto operator""_arr()
{ return parse_hex<fixcapvec,Digits...>().storage(); }
template<char... Digits> inline auto operator""_fcv()
{ return parse_hex<fixcapvec,Digits...>(); }
template<typename T, std::size_t> using vec = std::vector<T>;
template<char... Digits> inline auto operator""_vec()
{ return parse_hex<vec,Digits...>(); }
template<typename,   std::size_t> using str = std::string;
template<char... Digits> inline auto operator""_str()
{ return parse_hex<str,Digits...>(); }


template<typename K, typename C, typename T>
std::optional<std::string> unaesgcm(
  const K &key, const std::vector<byte> &iv, const C &ct, const T &tag )
{
  static_assert( std::size(T{})*byte_bits == tag_bits );
  const auto input = cat( ct, fixcapvec{tag} );
  std::istringstream in{std::string{std::begin(input),std::end(input)}};
  std::ostringstream out;
  if ( bool authentic = unaesgcm(iv, key, in, out) )
    return out.str();
  else
    return {};
}

int main()
{
  // The test vectors are from
  // https://csrc.nist.gov/CSRC/media/Projects/Cryptographic-Algorithm-Validation-Program/documents/mac/gcmtestvectors.zip

  // hex parsing
  {
    static_assert( byte_bits == 8 );
    auto ref_iv  = std::vector<byte>{
      0x47, 0x33, 0x60, 0xe0, 0xad, 0x24, 0x88, 0x99,
      0x59, 0x85, 0x89, 0x95,};
    constexpr
    auto ref_key = std::array<byte,bits<256>>{
      0x4c, 0x8e, 0xbf, 0xe1, 0x44, 0x4e, 0xc1, 0xb2,
      0xd5, 0x03, 0xc6, 0x98, 0x66, 0x59, 0xaf, 0x2c,
      0x94, 0xfa, 0xfe, 0x94, 0x5f, 0x72, 0xc1, 0xe8,
      0x48, 0x6a, 0x5a, 0xcf, 0xed, 0xb8, 0xa0, 0xf8};

    const auto [iv, key] = parse_iv_and_key(
      "473360e0ad248899"
      "59858995"

      "4c8ebfe1444ec1b2"
      "d503c6986659af2c"
      "94fafe945f72c1e8"
      "486a5acfedb8a0f8" );
    assert(( iv == ref_iv ));
    assert(( key == ref_key ));

    assert(( 0x473360e0ad24889959858995_vec == ref_iv ));
    assert(( 0x4c8ebfe1444ec1b2d503c6986659af2c94fafe945f72c1e8486a5acfedb8a0f8_arr == ref_key ));
  }

  // decryption of 16 octets
  {
    const auto Key = 0x4c8ebfe1444ec1b2d503c6986659af2c94fafe945f72c1e8486a5acfedb8a0f8_arr;
    const auto IV  = 0x473360e0ad24889959858995_vec;
    const auto CT  = 0xd2c78110ac7e8f107c0df0570bd7c90c_fcv;
    const auto Tag = 0xc26a379b6d98ef2852ead8ce83a833a7_arr;
    const auto PT  = 0x7789b41cb3ee548814ca0b388c10b343_str;
    assert(( unaesgcm(Key,IV,CT,Tag) == PT ));
  }
  {
    const auto Key = 0xc997768e2d14e3d38259667a6649079de77beb4543589771e5068e6cd7cd0b14_arr;
    const auto IV  = 0x835090aed9552dbdd45277e2_vec;
    const auto CT  = 0x9f6607d68e22ccf21928db0986be126e_fcv;
    const auto Tag = 0xf32617f67c574fd9f44ef76ff880ab9f_arr;
    assert(( not unaesgcm(Key,IV,CT,Tag) ));
  }
  
  // decryption of 51 octets
  {
    const auto Key = 0x4433db5fe066960bdd4e1d4d418b641c14bfcef9d574e29dcd0995352850f1eb_arr;
    const auto IV  = 0x0e396446655582838f27f72f_vec;
    const auto CT  = 0xb0d254abe43bdb563ead669192c1e57e9a85c51dba0f1c8501d1ce92273f1ce7e140dcfac94757fabb128caad16912cead0607_fcv;
    const auto Tag = 0xffd0b02c92dbfcfbe9d58f7ff9e6f506_arr;
    const auto PT  = 0xd602c06b947abe06cf6aa2c5c1562e29062ad6220da9bc9c25d66a60bd85a80d4fbcc1fb4919b6566be35af9819aba836b8b47_str;
    assert(( unaesgcm(Key,IV,CT,Tag) == PT ));
  }
  {
    const auto Key = 0x28ae911ee685872d906de12d7696351df8ef2234a74a95efa4ea15b327338fe0_arr;
    const auto IV  = 0x2fe6a815d4865181fade5fac_vec;
    const auto CT  = 0x1168442ef64656ef6577fb42c1919c84aae856388e4db9945bb8c9b8412bbe6458bc400444d5d2bf2630f83468f66f9e46e790_fcv;
    const auto Tag = 0xb75f616fd1a3d6563b62b899e5a7e522_arr;
    assert(( not unaesgcm(Key,IV,CT,Tag) ));
    }
}
