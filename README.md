
# unaesgcm
An AES-256-GCM de-/encryption utility specializing on `aesgcm`-scheme URLs as
described in the [OMEMO media sharing XMPP extension (XEP-0454)](https://xmpp.org/extensions/xep-0454.html)
and as implemented in modern XMPP clients. The latter meaning, in particular,
that the initialization vector size is not restricted to 96 bits.

The utility consists of:

* `unaesgcm` command for decrypting local files;
* `aesgcm-open` command for fetching URLs, decrypting and opening files with
associated applications;
* a desktop entry handling `aesgcm` URLs using a GUI-friendly flavor of the
above command;
* `aesgcm` command for encrypting local files that exists mostly for symmetry.

The utility is intended mainly for development/debugging, as the URLs with this
scheme are supposed to be invisible to XMPP users, in ideal.

### Some alternatives

[In Rust](https://github.com/moparisthebest/ImageDownloader),
[C](https://github.com/wstrm/omemo-utils),
[C](https://github.com/moparisthebest/ImageDownloader/blob/master/aesgcm.c),
[Go](https://github.com/roobre/omemo-wget),
[Java](https://github.com/iNPUTmice/ImageDownloader).
Except, notably, omemo-utils, they seem to be more bare-bones, but may be
preferred for their respective implementation languages and, therefore, the
required tools and libraries.


## Requirements

* A C++20-ish compiler. g++ 8 or clang++ 9 will do.
* GNU Make (not *strictly* necessary – unaesgcm is not terribly hard to build
and install manually – but the instructions below rely on it).

For Debian 10, the package names are `g++`, `clang-9` and `make`, respectively.

### Build-time dependencies

* `libcrypto`

For Debian(-derived) systems: `# apt install libssl-dev`.

### Install-time dependencies

* desktop-file-utils

For Debian(-derived) systems: `# apt install desktop-file-utils`

### Run-time dependencies

* `libcrypto`, for de-/encryption;
* `curl`, for URL fetching;
* xdg-utils, for opening decrypted files;
* optionally, `gtk-launch`, for opening decrypted files using, specifically,
the content type reported by the server;
* optionally, `zenity`, for interactive prompt of decryption key and IV when a
URL doesn't contain them (i. e. lacks the `#fragment` part).

For Debian(-derived) systems: `# apt install libssl1.1 curl xdg-utils
libgtk-3-bin zenity`


## Installation

```
$ make test && (./test && echo "ok!" || echo "fail!")
$ make
# make install
```

The latter places the files under `/usr/local` (use `prefix` and `DESTDIR`
variables to override) and updates media type mappings cache. You, as a user,
may additionaly need to do

`$ xdg-mime default unaesgcm.desktop x-scheme-handler/aesgcm`

to make the provided desktop entry the *default* `aesgcm` handler (for example,
if you happen to have some *other* handler(s) already installed). To uninstall:

`# make uninstall`

## Usage

```
xdg-open AESGCM-URL
aesgcm-open URL [CONTENT-TYPE-OVERRIDE]
[un]aesgcm IN-FILE OUT-FILE IV-KEY
```

### Examples

`xdg-open    aesgcm://files.xmpp.example.org/7/r/7rEFTd6cxUI#473360e0a\
d248899598589954c8ebfe1444ec1b2d503c6986659af2c94fafe945f72c1e8486a5acfedb8a0f8`

`xdg-open    aesgcm://files.xmpp.example.org/7/r/7rEFTd6cxUI`  
*\*graphical key entry dialog pops up\**

`aesgcm-open aesgcm://files.xmpp.example.org/7/r/7rEFTd6cxUI#473360e0a\
d248899598589954c8ebfe1444ec1b2d503c6986659af2c94fafe945f72c1e8486a5acfedb8a0f8`

`aesgcm-open  https://files.xmpp.example.org/7/r/7rEFTd6cxUI#473360e0a\
d248899598589954c8ebfe1444ec1b2d503c6986659af2c94fafe945f72c1e8486a5acfedb8a0f8`

```
$ cd ~/Downloads
$ wget -q https://files.xmpp.example.org/7/r/7rEFTd6cxUI#fragment-is-not-used
$ aesgcm-open file://$HOME/Downloads/7rEFTd6cxUI image/jpeg
Enter IV and key, concatenated, in hex (won't be echoed): 
overriding content type '' with 'image/jpeg'
IV size: 12 bytes
  % Total    % Received % Xferd  Average Speed   Time    Time     Time  Current
                                 Dload  Upload   Total   Spent    Left  Speed
100   42k  100   42k    0     0  42.0M      0 --:--:-- --:--:-- --:--:-- 42.0M
plaintext size: 43008 bytes
tag: 01-23-45-67-89-ab-cd-ef-01-23-45-67-89-ab-cd-ef
```  
*\*decrypted file is opened with the preferred application\**

```
$ curl -sS https://files.xmpp.example.org/M/Y/MYEqkJMJk1U | \
head -c 64K | \
unaesgcm /dev/stdin /dev/stdout 0e396446655582838f27f72f\
4433db5fe066960bdd4e1d4d418b641c14bfcef9d574e29dcd0995352850f1eb | \
ffprobe -i - |& \
grep Duration
IV size: 12 bytes
plaintext size: 65520 bytes
tag: fe-dc-ba-98-76-54-32-10-fe-dc-ba-98-76-54-32-10
authentication failed (input may have been tampered with, output is untrustworthy)
  Duration: 02:48:27.77, start: 0.000000, bitrate: N/A
```  
Note: the `authentication failed` line is due to the incomplet input that causes
`unaesgcm` to bogusly interpret the trailing 16 bytes as the authentication tag.
This UX admittedly needs polish, as it is only possible to authenticate complete
input.

## Security & privacy considerations

Currently no effort has been made at keeping the cryptographic keys or decrypted
data secure, except for not sending them away from the local machine.

Note also that the `aesgcm` command currently relies on externally-provided
encryption key and IV, making *the user* responsible for making sure those were
generated correctly (using a cryptographically-secure RNG, etc.).

## License

[3‐clause BSD](LICENSE.md)
