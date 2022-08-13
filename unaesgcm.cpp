
#include "unaesgcm.hpp"
#include "fixcapvec.hpp"
#include "overload.hpp"
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
  const aes_key &key,
  std::istream &in, std::ostream &out )
{
  using std::cerr; using std::clog;
  using std::data; using std::size;
  using    ::data; using    ::size;
  using std::integral_constant;

  #define checked( func, args ) [&]{ \
    if ( const auto res = func args ) \
      return res; \
    cerr << "error: " #func " failed\n"; \
    throw see_stderr{}; \
  }();

  const auto ctx = checked( EVP_CIPHER_CTX_new,() );
  const auto ctxfree = [&]{ EVP_CIPHER_CTX_free(ctx); };
  struct autoctxfree : decltype(ctxfree) { ~autoctxfree(){ (*this)(); } }
    do_autoctxfree{ctxfree};

  const auto cipher = std::visit( overload
  {
    []( const std::array<byte,bits<256>> & ){ return EVP_aes_256_gcm(); },
    []( const std::array<byte,bits<192>> & ){ return EVP_aes_192_gcm(); },
    []( const std::array<byte,bits<128>> & ){ return EVP_aes_128_gcm(); },
  }, key );
  checked(EVP_DecryptInit_ex,(ctx, cipher, nullptr, nullptr, nullptr));
  checked(EVP_CIPHER_CTX_ctrl,(ctx, EVP_CTRL_GCM_SET_IVLEN,
    to_int(size(iv),"IV size"), nullptr));
  checked(EVP_DecryptInit_ex,(ctx, nullptr, nullptr, data(key), data(iv)));

  in .exceptions( {} );
  out.exceptions( {} );

  constexpr auto tag_size    = integral_constant<unsigned,bits<tag_bits>>{};
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

  const auto decrypt = [&]( const auto in )
  {
    fixcapvec<byte,capacity(in)> out;
    int out_size;
    static_assert( capacity(in) <= int_max_u );
    checked(EVP_DecryptUpdate,( ctx, data(out), &out_size,
      data(in), static_cast<int>(size(in)) ));
    out.resize( static_cast<std::size_t>(out_size) );
    total_pt_size += size(out);
    if ( size(out) != size(in) )
    {
      cerr << "error: decrypt failed after "<< total_pt_size <<" bytes\n";
      throw see_stderr{};
    }
    return out;
  };

  const auto write = [&]( const auto buf )
  {
    assert( size(buf) <= ssize_max_u );  // FIXME: write in two calls
    out.write(
      reinterpret_cast<const char *>(data(buf)),
      static_cast<std::streamsize>(size(buf)) );
    if ( not out )
    {
      const auto total_written = out.rdbuf()->pubseekoff(
        0, std::ios_base::cur, std::ios_base::out );
      cerr << "error: write failed after "<< total_written <<" bytes\n";
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
