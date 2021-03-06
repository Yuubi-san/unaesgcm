
#include "unaesgcm.hpp"
#include "fixcapvec.hpp"
#include <openssl/evp.h>
#include <iostream>
#include <cassert>

template<typename Cont>
constexpr auto capacity( const Cont &c )  { return c.capacity(); }
template<typename T, std::size_t N>
constexpr auto capacity( const T (&)[N] ) { return N; }

struct see_stderr : std::exception
{
  const char *what() const noexcept override
  {
    return "see stderr for details";
  }
};

[[maybe_unused]]
constexpr auto ssize_max_u = std::size_t{
  std::numeric_limits<std::streamsize>::max() };
constexpr auto   int_max_u =    unsigned{
  std::numeric_limits<            int>::max() };
auto to_int( const std::size_t sz, const std::string_view desc )
{
  if ( sz <= int_max_u )
    return static_cast<int>(sz);
  std::cerr << "error: "<< desc <<" doesn't fit into an int\n";
  throw see_stderr{};
}

bool unaesgcm( const std::vector<byte> &iv,
  const std::array<byte,key_bytes> &key,
  std::istream &in, std::ostream &out )
{
  using std::cerr; using std::clog;
  using std::data; using std::size;
  using std::integral_constant;

  #define checked( func, args ) [&]{ if ( const auto res = func args ) \
    return res; cerr << "error: " #func " failed\n"; throw see_stderr{}; }();

  const auto ctx = checked( EVP_CIPHER_CTX_new,() );
  const auto ctxfree = [&]{ EVP_CIPHER_CTX_free(ctx); };
  struct autoctxfree : decltype(ctxfree) { ~autoctxfree(){ (*this)(); } }
    do_autoctxfree{ctxfree};

  static_assert( key_bits == 256 );
  checked(EVP_DecryptInit_ex,(ctx, EVP_aes_256_gcm(), nullptr,nullptr,nullptr));
  checked(EVP_CIPHER_CTX_ctrl,(ctx, EVP_CTRL_GCM_SET_IVLEN,
    to_int(size(iv),"IV size"), nullptr));
  checked(EVP_DecryptInit_ex,(ctx, nullptr, nullptr, data(key), data(iv)));

  static_assert( tag_bits % byte_bits == 0 );
  constexpr auto tag_size    = integral_constant<unsigned,tag_bits/byte_bits>{};
  constexpr auto buffer_size = integral_constant<unsigned,4*1024>{};
  static_assert( buffer_size >= tag_size );

  std::uintmax_t total_read{}, total_pt_size{};

  const auto read = [&]( const auto N )
  {
    fixcapvec<byte,N> buf;
    in.read( reinterpret_cast<char *>(data(buf)), capacity(buf) );
    buf.resize( static_cast<std::size_t>(in.gcount()) );
    total_read += size(buf);
    if ( not in.eof() )
    {
      if ( size(buf) != capacity(buf) )
      {
        cerr << "error: read failed after "<< total_read <<" bytes\n";
        throw see_stderr{};
      }
    }
    return buf;
  };

  const auto decrypt = [&]( const auto ct )
  {
    fixcapvec<byte,capacity(ct)> pt;
    int pt_size;
    static_assert( capacity(ct) <= int_max_u );
    checked(EVP_DecryptUpdate,( ctx, data(pt), &pt_size,
      data(ct), static_cast<int>(size(ct)) ));
    pt.resize( static_cast<std::size_t>(pt_size) );
    total_pt_size += size(pt);
    if ( size(pt) != size(ct) )
    {
      cerr << "error: decrypt failed after "<< total_pt_size <<" bytes\n";
      throw see_stderr{};
    }
    return pt;
  };

  const auto write = [&]( const auto pt )
  {
    assert( size(pt) <= ssize_max_u );  // FIXME: write in two calls
    out.write(
      reinterpret_cast<const char *>(data(pt)),
      static_cast<std::streamsize>(size(pt)) );
    if ( not out )
    {
      cerr << "error: write failed after "<< out.tellp() <<" bytes\n";
      throw see_stderr{};
    }
  };

  const auto finalize = [&]<typename Cont>( Cont &&ct_tail_and_tag )
  {
    assert( size(ct_tail_and_tag) >= tag_size );
    const auto  ct =  pop_back(ct_tail_and_tag,  tag_size);
    const auto tag = tail(std::forward<Cont>(ct_tail_and_tag), tag_size);
    write(decrypt(ct));

    clog << "plaintext size: "<< total_pt_size <<" bytes\n";
    dump_hex( clog << "tag: ", tag) << '\n';

    checked(EVP_CIPHER_CTX_ctrl,( ctx, EVP_CTRL_GCM_SET_TAG,
      tag_size, const_cast<byte *>(data(tag)) ));

    int zero;
    return EVP_DecryptFinal_ex(ctx, nullptr, &zero) == 1;
  };

  #undef checked

  for ( fixcapvec<byte,tag_size> lookbehind; ; )
  {
    auto input = read(buffer_size);
    if ( size(input) == buffer_size )
    {
      write(decrypt(lookbehind));
      lookbehind = {};
      const auto lookahead = read(tag_size);
      if ( size(lookahead) == tag_size )
      {
        write(decrypt(input));
        lookbehind = lookahead;
        continue;
      }
      else  // eof
        return finalize( cat(input,lookahead) );
    }
    else  // eof
    {
      auto ext_input = cat(lookbehind,input);
      if ( size(ext_input) >= tag_size )
        return finalize( std::move(ext_input) );
      else
      {
        cerr << "error: input too short ("<< total_read <<" bytes)\n";
        throw see_stderr{};
      }
    }
  }
}
