
#ifndef UNAESGCM_BASENAME_HPP
#define UNAESGCM_BASENAME_HPP

#include <algorithm>
#include <string_view>

inline std::string_view basename( const std::string_view path ) noexcept
{
  const auto found = std::find_if( std::rbegin(path), std::rend(path),
    [](const auto c){ return c == '/' or c == '\\'; } ).base();
  return { found, std::end(path) };
}

#endif
