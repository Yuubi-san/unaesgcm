
#include "unaesgcm.hpp"
#include <iostream>

int main( const int argc, const char *const *const argv )
{
  if ( argc != 2 )
  {
    std::clog <<
      "usage:   "<< argv[0] <<" hex_IV|hex_256bit_key\n"
      "example: "<< argv[0] <<" 8d82e8083d601e7f67de918418ef6c3cb703ebb91c7a943"
        "b9285eb5049fc319da4127bd6df34fedec8c58b71\n";
    return 2;
  }
  const auto [iv, key] = parse_iv_and_key( argv[1] );
  std::clog << "IV size: "<< size(iv) <<" bytes\n";
  if ( unaesgcm(iv, key, std::cin, std::cout) )
    return 0;
  else
  {
    std::clog <<
      "authentication failed (input may have been tampered with, "
      "output is untrustworthy)\n";
    return 1;
  }
}
