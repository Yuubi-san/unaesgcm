
#include "aesgcm.hpp"
#include "fixcapvec.hpp"
#include "overload.hpp"
#include <openssl/evp.h>
#include <iostream>
#include <tuple>
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

static auto aesgcm( auto decrypt,
  const std::vector<byte> &iv, const aes_key &key,
  std::istream &in, std::ostream &out )
{
  using std::cerr; using std::clog;
  using std::data; using std::size;
  using    ::data; using    ::size;
  using std::integral_constant;

  const auto &[verb, EVP_Init_ex, EVP_Update] = not decrypt ?
    std::tie("encrypt", EVP_EncryptInit_ex, EVP_EncryptUpdate):
    std::tie("decrypt", EVP_DecryptInit_ex, EVP_DecryptUpdate);

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
  checked(EVP_Init_ex,(ctx, cipher, nullptr, nullptr, nullptr));
  checked(EVP_CIPHER_CTX_ctrl,(ctx, EVP_CTRL_GCM_SET_IVLEN,
    to_int(size(iv),"IV size"), nullptr));
  checked(EVP_Init_ex,(ctx, nullptr, nullptr, data(key), data(iv)));

  in .exceptions( {} );
  out.exceptions( {} );

  constexpr auto tag_size    = integral_constant<unsigned,bits<tag_bits>>{};
  constexpr auto buffer_size = integral_constant<unsigned,4*1024>{};
  static_assert( buffer_size >= tag_size );

  std::uintmax_t total_read{}, total_processed{};

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

  const auto update = [&]( const auto in )
  {
    fixcapvec<byte,capacity(in)> out;
    int out_size;
    static_assert( capacity(in) <= int_max_u );
    checked(EVP_Update,( ctx, data(out), &out_size,
      data(in), static_cast<int>(size(in)) ));
    out.resize( static_cast<std::size_t>(out_size) );
    total_processed += size(out);
    if ( size(out) != size(in) )
    {
      cerr <<
        "error: "<< verb <<" failed after "<< total_processed <<" bytes\n";
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

  const auto finalize_enc = [&]
  {
    int zero;
    checked(EVP_EncryptFinal_ex,( ctx, nullptr, &zero ));

    clog << "ciphertext size: "<< total_processed <<" bytes\n";

    std::array<byte,tag_size> tag;
    checked(EVP_CIPHER_CTX_ctrl,( ctx, EVP_CTRL_AEAD_GET_TAG,
      size(tag), data(tag) ));

    dump_hex(clog << "tag: ", tag) << '\n';
    write(tag);
    return true;
  };

  const auto finalize_dec = [&]<typename Cont>( Cont &&ct_tail_and_tag )
  {
    assert( size(ct_tail_and_tag) >= tag_size );
    const auto  ct =  pop_back(ct_tail_and_tag,  tag_size);
    const auto tag = tail(std::forward<Cont>(ct_tail_and_tag), tag_size);
    write(update(ct));

    clog << "plaintext size: "<< total_processed <<" bytes\n";
    dump_hex( clog << "tag: ", tag) << '\n';

    checked(EVP_CIPHER_CTX_ctrl,( ctx, EVP_CTRL_GCM_SET_TAG,
      tag_size, const_cast<byte *>(data(tag)) ));

    int zero;
    return EVP_DecryptFinal_ex(ctx, nullptr, &zero) == 1;
  };

  #undef checked

  if ( not decrypt ) for (;;)
  {
    auto input = read(buffer_size);
    const auto eof = size(input) != buffer_size;
    write(update( std::move(input) ));
    if ( eof )
      return finalize_enc();
  }
  else for ( fixcapvec<byte,tag_size> lookbehind; ; )
  {
    auto input = read(buffer_size);
    if ( size(input) == buffer_size )
    {
      write(update(lookbehind));
      lookbehind = {};
      const auto lookahead = read(tag_size);
      if ( size(lookahead) == tag_size )
      {
        write(update(input));
        lookbehind = lookahead;
        continue;
      }
      else  // eof
        return finalize_dec( cat(input,lookahead) );
    }
    else  // eof
    {
      auto ext_input = cat(lookbehind,input);
      if ( size(ext_input) >= tag_size )
        return finalize_dec( std::move(ext_input) );
      else
      {
        cerr << "error: input too short ("<< total_read <<" bytes)\n";
        throw see_stderr{};
      }
    }
  }
}

enum verb : bool { encrypt, decrypt };

void aesgcm(
  const std::vector<byte> &iv, const aes_key &key,
  std::istream &in, std::ostream &out )
{ aesgcm(std::integral_constant<verb,encrypt>{}, iv, key, in, out); }

bool unaesgcm(
  const std::vector<byte> &iv, const aes_key &key,
  std::istream &in, std::ostream &out )
{ return aesgcm(std::integral_constant<verb,decrypt>{}, iv, key, in, out); }
