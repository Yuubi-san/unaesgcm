
#ifndef UNAESGCM_BASENAME_HPP
#define UNAESGCM_BASENAME_HPP

#include <algorithm>
#include <string_view>
#include <type_traits>

// for extra confusion, complies with neither POSIX nor GNU; see tests below
template<typename RecognizeBackslash =
#ifndef _WIN32
  std::false_type
#else
  std::true_type
#endif
>
constexpr std::string_view basename(
  const std::string_view path, const RecognizeBackslash rcgnz_bs = {} ) noexcept
{
  const auto found = std::find_if( std::rbegin(path), std::rend(path),
    [&](const auto c){ return c == '/' or (rcgnz_bs and c == '\\'); } ).base();
  return { found, static_cast<std::size_t>(std::end(path) - found) };
}

static_assert( basename(std::string_view{""}) == "" );
static_assert( basename(std::string_view{"/"}) == "" );
static_assert( basename(std::string_view{"foo/"}) == "" );
static_assert( basename(std::string_view{"foo"}) == "foo" );
static_assert( basename(std::string_view{"foo/bar"}) == "bar" );

static_assert( basename("foo/bar\\baz", false) == "bar\\baz" );
static_assert( basename("foo/bar\\baz", true ) ==      "baz" );

#endif
