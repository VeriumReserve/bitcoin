Verium Core
=============

Setup
---------------------
Verium Core is the original Verium client and it builds the backbone of the network. It downloads and, by default, stores the entire history of Verium transactions; depending on the speed of your computer and network connection, the synchronization process can take anywhere from a few hours to a day or more.

In order to help you to be able to sync as fast as possible we have implemented a bootstrap system explain in the documentation below

To download Verium Core, visit [vericoin.info](https://vericoin.info/verium-digital-reserve/).

Running
---------------------
The following are some helpful notes on how to run Verium on your native platform.

### Unix

Unpack the files into a directory and run:

- `bin/verium-qt` (GUI) or
- `bin/veriumd` (headless)

### Windows

Download and execute "Verium_0.16_64bit.exe"

### OS X

Drag Verium_0.16_64bit.pkg to your applications folder, and then run Verium Wallet.

Bootstrap
---------------------
In order to speed up the download of the blockchain and to let you discover the full potential of the Verium Wallet, a bootstrap process is implemented in the Verium Core.

* You can launch it by different methods base on how you are using you wallet:
  - Command line: src/verium-cli bootstrap
  - UI: file > reload blockchain


### Need Help?

* See the documentation at the [Vericoin & Verium Wiki](https://wiki.vericoin.info/)
for help and more information.
* Ask for help on 
 - [Slack](https://slack.vericoin.info)
 - [Telegram](https://t.me/vericoinandverium)
 - [Vericoin & Verium Reddit](https://www.reddit.com/r/vericoin)

Building
---------------------
The following are developer notes on how to build Verium on your native platform. They are not complete guides, but include notes on the necessary libraries, compile flags, etc.

- [Dependencies](dependencies.md)
- [OS X Build Notes](build-osx.md)
- [Unix Build Notes](build-unix.md)
- [Windows Build Notes](build-windows.md)
- [OpenBSD Build Notes](build-openbsd.md)
- [Gitian Building Guide](gitian-building.md)

Development
---------------------
The Verium repo's [root README](/README.md) contains relevant information on the development process and automated testing.

- [Developer Notes](developer-notes.md)
- [Release Notes](release-notes.md)
- [Release Process](release-process.md)
- [Source Code Documentation (External Link)](https://dev.visucore.com/bitcoin/doxygen/)
- [Translation Process](translation_process.md)
- [Translation Strings Policy](translation_strings_policy.md)
- [Travis CI](travis-ci.md)
- [Unauthenticated REST Interface](REST-interface.md)
- [Shared Libraries](shared-libraries.md)
- [BIPS](bips.md)
- [Dnsseed Policy](dnsseed-policy.md)
- [Benchmarking](benchmarking.md)

### Resources
* Discuss on the [Slack](https://slack.vericoin.info), in the development channel.
* Follow the [Development Kanban](https://trello.com/b/Fna9ydfw/vericonomy).

### Miscellaneous
- [Assets Attribution](assets-attribution.md)
- [Files](files.md)
- [Fuzz-testing](fuzzing.md)
- [Reduce Traffic](reduce-traffic.md)
- [Tor Support](tor.md)
- [Init Scripts (systemd/upstart/openrc)](init.md)
- [ZMQ](zmq.md)

License
---------------------
Distributed under the [MIT software license](/COPYING).
This product includes software developed by the OpenSSL Project for use in the [OpenSSL Toolkit](https://www.openssl.org/). This product includes
cryptographic software written by Eric Young ([eay@cryptsoft.com](mailto:eay@cryptsoft.com)), and UPnP software written by Thomas Bernard.
