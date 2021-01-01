
# unaesgcm
An AES-256-GCM decryption utility specializing on `aesgcm`-scheme URLs as
described in the [OMEMO media sharing XMPP proto-extension](https://xmpp.org/extensions/inbox/omemo-media-sharing.html)
and as implemented in modern XMPP clients. The latter meaning, in particular,
that the initialization vector size is not restricted to 96 bits.

The utility consists of:

* `unaesgcm` command for decrypting local files;
* `aesgcm-open` command for fetching URLs, decrypting and opening files with
associated applications.

To be included is a `.desktop` file to associate the above with `aesgcm` scheme.

The utility is intended mainly for development/debugging, as the URLs with this
scheme are supposed to be invisible to XMPP users, in ideal.


## Requirements

A C++20-ish compiler. g++ 8 or clang++ 9 will do.

### Build-time dependencies

* `libcrypto`

For Debian(-derived) systems: `# apt install libssl-dev`.

### Run-time dependencies

* `libssl`, for decryption;
* `curl`, for URL fetching;
* xdg-utils, for opening decrypted files;
* optionally, `gtk-launch`, for opening decrypted files using, specifically,
the content type reported by the server.

For Debian(-derived) systems: `# apt install libssl1.1 curl xdg-utils
libgtk-3-bin`


## Installation

`$ make test && (./test && echo "ok!" || echo "fail!")`<br/>
`$ make`<br/>
`# make install`

The latter places the files under `/usr/local`. To uninstall:

`# make uninstall`

## Security & privacy considerations

Currently no effort has been made at keeping the decryption key or decrypted
data secure, except for not sending them away from the local machine.

## License

[3‐clause BSD](LICENSE.md)
