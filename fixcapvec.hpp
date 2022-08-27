
#ifndef UNAESGCM_FIXCAPVEC_HPP
#define UNAESGCM_FIXCAPVEC_HPP

#include <array>
#include <iterator>
#include <algorithm>
#include <cassert>

template<typename T, std::size_t Capacity>
class fixcapvec
{
  std::array<T,Capacity> _data;
  std::size_t            _size = 0;
public:
  constexpr fixcapvec() {}
  constexpr fixcapvec( const std::array<T,Capacity> &a )
    : _data{a}
    , _size{std::size(_data)}
  {}
  constexpr const auto &storage() const { return _data; }
  constexpr auto begin() const { return std::begin(_data); }
  constexpr auto begin()       { return std::begin(_data); }
  constexpr auto   end() const { return begin()+size(); }
  constexpr auto   end()       { return begin()+size(); }
  constexpr auto  data() const { return  std::data(_data); }
  constexpr auto  data()       { return  std::data(_data); }
  constexpr auto  size() const { return _size; }
  constexpr bool empty() const { return !size(); }
  constexpr void resize(std::size_t sz) {assert(sz <= capacity()); _size = sz;}
  static
  constexpr auto capacity() { return std::size(decltype(_data){}); }
  static
  constexpr auto max_size() { return capacity(); }
};

template<typename T, std::size_t CL, std::size_t CR>
constexpr auto cat( const fixcapvec<T,CL> &l, const fixcapvec<T,CR> &r )
{
  using std::data; using std::size; using std::copy;
  fixcapvec<T,CL+CR> ret;
  copy( data(l), data(l)+size(l), data(ret) );
  copy( data(r), data(r)+size(r), data(ret)+size(l) );
  ret.resize( size(l) + size(r) );
  return ret;
}

template<typename T, std::size_t C>
inline auto tail( fixcapvec<T,C> &&v, const std::size_t n )
{
  using std::data; using std::size; using std::copy;
  assert( n <= size(v) );
  std::copy( data(v)+size(v)-n, data(v)+size(v), data(v) );
  v.resize(n);
  return std::move(v);
}

template<typename T, std::size_t C>
inline auto head( fixcapvec<T,C> &&v, const std::size_t n )
{
  assert( n <= std::size(v) );
  v.resize(n);
  return std::move(v);
}
template<typename T, std::size_t C>
inline auto head( const fixcapvec<T,C> &v, const std::size_t n )
{
  using std::data; using std::size; using std::copy;
  fixcapvec<T,C> ret;
  copy( data(v), data(v)+size(v), data(ret) );
  ret.resize( n );
  return ret;
}

template<typename T, std::size_t C>
inline auto pop_front( fixcapvec<T,C> &&v, std::size_t n )
{
  assert( n <= std::size(v) );
  return tail( std::move(v), std::size(v)-n );
}
template<typename T, std::size_t C>
inline auto pop_back( const fixcapvec<T,C> &v, std::size_t n )
{
  assert( n <= std::size(v) );
  return head( v, std::size(v)-n );
}

#endif
