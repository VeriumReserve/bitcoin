Verium Vault version 1.3.0 is now available from:

  https://vericoin.info

This is a new major version release, bringing both new features and
bug fixes.

Please report bugs using the issue tracker at github:

  https://github.com/VeriumReserve/Verium

Upgrading and downgrading
=========================

How to Upgrade
--------------

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), uninstall all
earlier versions of Verium, then run the installer.

We recommand before any upgrade that you backup your wallet.

If you are upgrading from version 1.2 or earlier, the first time you run
1.3.0 your blockchain files will be re-indexed, which will take anywhere from
5 minutes to several hours, depending on the speed of your machine.

Windows 64-bit installer
-------------------------

New in 1.3.0 is the Windows 64-bit version of the client. There have been
frequent reports of users running out of virtual memory on 32-bit systems
during the initial sync. Because of this it is recommended to install the
64-bit version if your system supports it.


Downgrading warnings
--------------------

The 'chainstate' for this release is not always compatible with previous
releases, so if you run 1.3.0 and then decide to switch back to a
1.2.x release you might get a blockchain validation error when starting the
old release (due to 'pruned outputs' being omitted from the index of
unspent transaction outputs).

Running the old release with the -reindex option will rebuild the chainstate
data structures and correct the problem.

Also, the first time you run a 1.2.x release on a 1.3.0 wallet it will rescan
the blockchain for missing spent coins, which will take a long time (tens
of minutes on a typical machine).

Notable changes
===============

Autotools build system
-----------------------

For 1.3.0 we switched to an autotools-based build system instead of individual
(q)makefiles.

Using the standard "./autogen.sh; ./configure; make" to build Verium-Qt and
veriumd makes it easier for experienced open source developers to contribute
to the project.

Be sure to check doc/build-*.md for your platform before building from source.

Verium-cli
-------------

Another change in the 1.3.0 release is moving away from the veriumd executable
functioning both as a server and as a RPC client. The RPC client functionality
("tell the running verium daemon to do THIS") was split into a separate
executable, 'verium-cli'.

`walletpassphrase` RPC
-----------------------

The behavior of the `walletpassphrase` RPC when the wallet is already unlocked
has changed between 1.2.x and 1.3.0.

The 1.2.x behavior of `walletpassphrase` is to fail when the wallet is already unlocked:

    > walletpassphrase 1000
    walletunlocktime = now + 1000
    > walletpassphrase 10
    Error: Wallet is already unlocked (old unlock time stays)

The new behavior of `walletpassphrase` is to set a new unlock time overriding
the old one:

    > walletpassphrase 1000
    walletunlocktime = now + 1000
    > walletpassphrase 10
    walletunlocktime = now + 10 (overriding the old unlock time)

Faster synchronization
----------------------

Verium now uses 'headers-first synchronization'. This means that we first
ask peers for block headers and validate those.
In a second stage, when the headers have been discovered, we
download the blocks. However, as we already know about the whole chain in
advance, the blocks can be downloaded in parallel from all available peers.

In practice, this means a much faster and more robust synchronization. On
recent hardware with a decent network link, it can be as little as 3 hours
for an initial full synchronization. You may notice a slower progress in the
very first few minutes, when headers are still being fetched and verified, but
it should gain speed afterwards.

A few RPCs were added/updated as a result of this:
- `getblockchaininfo` now returns the number of validated headers in addition to
the number of validated blocks.
- `getpeerinfo` lists both the number of blocks and headers we know we have in
common with each peer. While synchronizing, the heights of the blocks that we
have requested from peers (but haven't received yet) are also listed as
'inflight'.

RPC access control changes
--------------------------

Subnet matching for the purpose of access control is now done
by matching the binary network address, instead of with string wildcard matching.
For the user this means that `-rpcallowip` takes a subnet specification, which can be

- a single IP address (e.g. `1.2.3.4` or `fe80::0012:3456:789a:bcde`)
- a network/CIDR (e.g. `1.2.3.0/24` or `fe80::0000/64`)
- a network/netmask (e.g. `1.2.3.4/255.255.255.0` or `fe80::0012:3456:789a:bcde/ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff`)

An arbitrary number of `-rpcallow` arguments can be given. An incoming connection will be accepted if its origin address
matches one of them.

For example:

| 1.2.x and before                           | 1.3.x                                |
|--------------------------------------------|---------------------------------------|
| `-rpcallowip=192.168.1.1`                  | `-rpcallowip=192.168.1.1` (unchanged) |
| `-rpcallowip=192.168.1.*`                  | `-rpcallowip=192.168.1.0/24`          |
| `-rpcallowip=192.168.*`                    | `-rpcallowip=192.168.0.0/16`          |
| `-rpcallowip=*` (dangerous!)               | `-rpcallowip=::/0` (still dangerous!) |

Using wildcards will result in the rule being rejected with the following error in debug.log:

    Error: Invalid -rpcallowip subnet specification: *. Valid are a single IP (e.g. 1.2.3.4), a network/netmask (e.g. 1.2.3.4/255.255.255.0) or a network/CIDR (e.g. 1.2.3.4/24).


REST interface
--------------

A new HTTP API is exposed when running with the `-rest` flag, which allows
unauthenticated access to public node data.

It is served on the same port as RPC, but does not need a password, and uses
plain HTTP instead of JSON-RPC.

Assuming a local RPC server running on port 33987, it is possible to request:
- Blocks: http://localhost:33987/rest/block/*HASH*.*EXT*
- Blocks without transactions: http://localhost:33987/rest/block/notxdetails/*HASH*.*EXT*
- Transactions (requires `-txindex`): http://localhost:33987/rest/tx/*HASH*.*EXT*

In every case, *EXT* can be `bin` (for raw binary data), `hex` (for hex-encoded
binary) or `json`.

For more details, see the `doc/REST-interface.md` document in the repository.

RPC Server "Warm-Up" Mode
-------------------------

The RPC server is started earlier now, before most of the expensive
intialisations like loading the block index.  It is available now almost
immediately after starting the process.  However, until all initialisations
are done, it always returns an immediate error with code -28 to all calls.

This new behaviour can be useful for clients to know that a server is already
started and will be available soon (for instance, so that they do not
have to start it themselves).

Improved signing security
-------------------------

For 1.3.0 the security of signing against unusual attacks has been
improved by making the signatures constant time and deterministic.

This change is a result of switching signing to use libsecp256k1
instead of OpenSSL. Libsecp256k1 is a cryptographic library
optimized for the curve Bitcoin uses which was created by Bitcoin
Core developer Pieter Wuille.

There exist attacks[1] against most ECC implementations where an
attacker on shared virtual machine hardware could extract a private
key if they could cause a target to sign using the same key hundreds
of times. While using shared hosts and reusing keys are inadvisable
for other reasons, it's a better practice to avoid the exposure.

OpenSSL has code in their source repository for derandomization
and reduction in timing leaks that we've eagerly wanted to use for a
long time, but this functionality has still not made its
way into a released version of OpenSSL. Libsecp256k1 achieves
significantly stronger protection: As far as we're aware this is
the only deployed implementation of constant time signing for
the curve Bitcoin uses and we have reason to believe that
libsecp256k1 is better tested and more thoroughly reviewed
than the implementation in OpenSSL.

[1] https://eprint.iacr.org/2014/161.pdf

Watch-only wallet support
-------------------------

The wallet can now track transactions to and from wallets for which you know
all addresses (or scripts), even without the private keys.

This can be used to track payments without needing the private keys online on a
possibly vulnerable system. In addition, it can help for (manual) construction
of multisig transactions where you are only one of the signers.

One new RPC, `importaddress`, is added which functions similarly to
`importprivkey`, but instead takes an address or script (in hexadecimal) as
argument.  After using it, outputs credited to this address or script are
considered to be received, and transactions consuming these outputs will be
considered to be sent.

The following RPCs have optional support for watch-only:
`getbalance`, `listreceivedbyaddress`, `listreceivedbyaccount`,
`listtransactions`, `listaccounts`, `listsinceblock`, `gettransaction`. See the
RPC documentation for those methods for more information.

Compared to using `getrawtransaction`, this mechanism does not require
`-txindex`, scales better, integrates better with the wallet, and is compatible
with future block chain pruning functionality. It does mean that all relevant
addresses need to added to the wallet before the payment, though.

Consensus library
-----------------

Starting from 1.3.0, the Verium distribution includes a consensus library.

The purpose of this library is to make the verification functionality that is
critical to Verium's consensus available to other applications.

This library is called `libbitcoinconsensus.so` (or, `.dll` for Windows).
Its interface is defined in the C header [bitcoinconsensus.h](https://github.com/VeriumReserve/verium/blob/1.3.0/src/script/bitcoinconsensus.h).

In its initial version the API includes two functions:

- `bitcoinconsensus_verify_script` verifies a script. It returns whether the indicated input of the provided serialized transaction
correctly spends the passed scriptPubKey under additional constraints indicated by flags
- `bitcoinconsensus_version` returns the API version, currently at an experimental `0`

The functionality is planned to be extended to e.g. UTXO management in upcoming releases, but the interface
for existing methods should remain stable.

verium-tx
----------

It has been observed that many of the RPC functions offered by veriumd are
"pure functions", and operate independently of the veriumd wallet. This
included many of the RPC "raw transaction" API functions, such as
createrawtransaction.

verium-tx is a newly introduced command line utility designed to enable easy
manipulation of verium transactions. A summary of its operation may be
obtained via "verium-tx --help" Transactions may be created or signed in a
manner similar to the RPC raw tx API. Transactions may be updated, deleting
inputs or outputs, or appending new inputs and outputs. Custom scripts may be
easily composed using a simple text notation, borrowed from the verium test
suite.

This tool may be used for experimenting with new transaction types, signing
multi-party transactions, and many other uses. Long term, the goal is to
deprecate and remove "pure function" RPC API calls, as those do not require a
server round-trip to execute.

Fix buffer overflow in bundled upnp
------------------------------------

Bundled miniupnpc was updated to 1.9.20151008. This fixes a buffer overflow in
the XML parser during initial network discovery.

Details can be found here: http://talosintel.com/reports/TALOS-2015-0035/

This applies to the distributed executables only, not when building from source or
using distribution provided packages.

Additionally, upnp has been disabled by default. This may result in a lower
number of reachable nodes on IPv4, however this prevents future libupnpc
vulnerabilities from being a structural risk to the network

Block file pruning
----------------------

This release supports running a fully validating node without maintaining a copy
of the raw block and undo data on disk. To recap, there are four types of data
related to the blockchain in the verium system: the raw blocks as received over
the network (blk???.dat), the undo data (rev???.dat), the block index and the
UTXO set (both LevelDB databases). The databases are built from the raw data.

Block pruning allows Verium to delete the raw block and undo data once
it's been validated and used to build the databases. At that point, the raw data
is used only to relay blocks to other nodes, to handle reorganizations, to look
up old transactions (if -txindex is enabled or via the RPC/REST interfaces), or
for rescanning the wallet. The block index continues to hold the metadata about
all blocks in the blockchain.

The user specifies how much space to allot for block & undo files. The minimum
allowed is 550MB. Note that this is in addition to whatever is required for the
block index and UTXO databases. The minimum was chosen so that Verium will
be able to maintain at least 288 blocks on disk (two days worth of blocks at 10
minutes per block). In rare instances it is possible that the amount of space
used will exceed the pruning target in order to keep the required last 288
blocks on disk.

Block pruning works during initial sync in the same way as during steady state,
by deleting block files "as you go" whenever disk space is allocated. Thus, if
the user specifies 550MB, once that level is reached the program will begin
deleting the oldest block and undo files, while continuing to download the
blockchain.

For now, block pruning disables block relay.  In the future, nodes with block
pruning will at a minimum relay "new" blocks, meaning blocks that extend their
active chain.

Block pruning is currently incompatible with running a wallet due to the fact
that block data is used for rescanning the wallet and importing keys or
addresses (which require a rescan.) However, running the wallet with block
pruning will be supported in the near future, subject to those limitations.

Block pruning is also incompatible with -txindex and will automatically disable
it.

Once you have pruned blocks, going back to unpruned state requires
re-downloading the entire blockchain. To do this, re-start the node with
-reindex. Note also that any problem that would cause a user to reindex (e.g.,
disk corruption) will cause a pruned node to redownload the entire blockchain.
Finally, note that when a pruned node reindexes, it will delete any blk???.dat
and rev???.dat files in the data directory prior to restarting the download.

To enable block pruning on the command line:

- `-prune=N`: where N is the number of MB to allot for raw block & undo data.

Modified RPC calls:

- `getblockchaininfo` now includes whether we are in pruned mode or not.
- `getblock` will check if the block's data has been pruned and if so, return an
error.
- `getrawtransaction` will no longer be able to locate a transaction that has a
UTXO but where its block file has been pruned.

Pruning is disabled by default.

Big endian support
--------------------

Experimental support for big-endian CPU architectures was added in this
release. All little-endian specific code was replaced with endian-neutral
constructs. This has been tested on at least MIPS and PPC hosts. The build
system will automatically detect the endianness of the target.

Memory usage optimization
--------------------------

There have been many changes in this release to reduce the default memory usage
of a node, among which:

- Accurate UTXO cache size accounting; this makes the option `-dbcache`
  precise where this grossly underestimated memory usage before
- Reduce size of per-peer data structure; this increases the
  number of connections that can be supported with the same amount of memory
- Reduce the number of threads; lowers the amount of (esp.
  virtual) memory needed

Privacy: Stream isolation for Tor
----------------------------------

This release adds functionality to create a new circuit for every peer
connection, when the software is used with Tor. The new option,
`-proxyrandomize`, is on by default.

When enabled, every outgoing connection will (potentially) go through a
different exit node. That significantly reduces the chance to get unlucky and
pick a single exit node that is either malicious, or widely banned from the P2P
network. This improves connection reliability as well as privacy, especially
for the initial connections.

**Important note:** If a non-Tor SOCKS5 proxy is configured that supports
authentication, but doesn't require it, this change may cause that proxy to reject
connections. A user and password is sent where they weren't before. This setup
is exceedingly rare, but in this case `-proxyrandomize=0` can be passed to
disable the behavior.

Other P2P Changes
-----------------

The list of banned peers is now stored on disk rather than in memory.
Restarting veriumd will no longer clear out the list of banned peers; instead
a new RPC call (`clearbanned`) can be used to manually clear the list.  The new
`setban` RPC call can also be used to manually ban or unban a peer.

Mining Code Changes
-------------------

The mining code in 1.3.0 has been optimized to be significantly faster and use less
memory. As part of these changes, consensus critical calculations are cached on a
transaction's acceptance into the mempool and the mining code now relies on the
consistency of the mempool to assemble blocks. However all blocks are still tested
for validity after assembly.

Option parsing behavior
-----------------------

Command line options are now parsed strictly in the order in which they are
specified. It used to be the case that `-X -noX` ends up, unintuitively, with X
set, as `-X` had precedence over `-noX`. This is no longer the case. Like for
other software, the last specified value for an option will hold.

Signature validation using libsecp256k1
---------------------------------------

ECDSA signatures inside Verium transactions now use validation using
[libsecp256k1](https://github.com/bitcoin-core/secp256k1) instead of OpenSSL.

Depending on the platform, this means a significant speedup for raw signature
validation speed. The advantage is largest on x86_64, where validation is over
five times faster. In practice, this translates to a raw reindexing and new
block validation times that are less than half of what it was before.

Libsecp256k1 has undergone very extensive testing and validation.

A side effect of this change is that libconsensus no longer depends on OpenSSL.

Reduce upload traffic
---------------------

A major part of the outbound traffic is caused by serving historic blocks to
other nodes in initial block download state.

It is now possible to reduce the total upload traffic via the `-maxuploadtarget`
parameter. This is *not* a hard limit but a threshold to minimize the outbound
traffic. When the limit is about to be reached, the uploaded data is cut by not
serving historic blocks (blocks older than one week).
Moreover, any SPV peer is disconnected when they request a filtered block.

This option can be specified in MiB per day and is turned off by default
(`-maxuploadtarget=0`).
The recommended minimum is 144 * MAX_BLOCK_SIZE (currently 144MB) per day.

Whitelisted peers will never be disconnected, although their traffic counts for
calculating the target.

A more detailed documentation about keeping traffic low can be found in
[/doc/reduce-traffic.md](/doc/reduce-traffic.md).

RPC: Random-cookie RPC authentication
-------------------------------------

When no `-rpcpassword` is specified, the daemon now uses a special 'cookie'
file for authentication. This file is generated with random content when the
daemon starts, and deleted when it exits. Its contents are used as
authentication token. Read access to this file controls who can access through
RPC. By default it is stored in the data directory but its location can be
overridden with the option `-rpccookiefile`.

This is similar to Tor's CookieAuthentication: see
https://www.torproject.org/docs/tor-manual.html.en

This allows running veriumd without having to do any manual configuration.

Relay: Any sequence of pushdatas in OP_RETURN outputs now allowed
-----------------------------------------------------------------

Previously OP_RETURN outputs with a payload were only relayed and mined if they
had a single pushdata. This restriction has been lifted to allow any
combination of data pushes and numeric constant opcodes (OP_1 to OP_16) after
the OP_RETURN. The limit on OP_RETURN output size is now applied to the entire
serialized scriptPubKey, 83 bytes by default. (the previous 80 byte default plus
three bytes overhead)

Relay: New and only new blocks relayed when pruning
---------------------------------------------------

When running in pruned mode, the client will now relay new blocks. When
responding to the `getblocks` message, only hashes of blocks that are on disk
and are likely to remain there for some reasonable time window (1 hour) will be
returned (previously all relevant hashes were returned).

Automatically use Tor hidden services
-------------------------------------

Starting with Tor version 0.2.7.1 it is possible, through Tor's control socket
API, to create and destroy 'ephemeral' hidden services programmatically.
Verium has been updated to make use of this.

This means that if Tor is running (and proper authorization is available),
Verium automatically creates a hidden service to listen on, without
manual configuration. Verium will also use Tor automatically to connect
to other .onion nodes if the control socket can be successfully opened. This
will positively affect the number of available .onion nodes and their usage.

This new feature is enabled by default if Verium is listening, and
a connection to Tor can be made. It can be configured with the `-listenonion`,
`-torcontrol` and `-torpassword` settings. To show verbose debugging
information, pass `-debug=tor`.

Notifications through ZMQ
-------------------------

Veriumd can now (optionally) asynchronously notify clients through a
ZMQ-based PUB socket of the arrival of new transactions and blocks.
This feature requires installation of the ZMQ C API library 4.x and
configuring its use through the command line or configuration file.
Please see [docs/zmq.md](/doc/zmq.md) for details of operation.

Database cache memory increased
--------------------------------

As a result of growth of the UTXO set, performance with the prior default
database cache of 100 MiB has suffered.
For this reason the default was changed to 300 MiB in this release.

For nodes on low-memory systems, the database cache can be changed back to
100 MiB (or to another value) by either:

- Adding `dbcache=100` in verium.conf
- Changing it in the GUI under `Options → Size of database cache`

Note that the database cache setting has the most performance impact
during initial sync of a node, and when catching up after downtime.

verium-cli: arguments privacy
------------------------------

The RPC command line client gained a new argument, `-stdin`
to read extra arguments from standard input, one per line until EOF/Ctrl-D.
For example:

    $ src/verium-cli -stdin walletpassphrase
    mysecretcode
    120
    ..... press Ctrl-D here to end input
    $

It is recommended to use this for sensitive information such as wallet
passphrases, as command-line arguments can usually be read from the process
table by any user on the system.


C++11 and Python 3
------------------

Various code modernizations have been done. The Verium code base has
started using C++11. This means that a C++11-capable compiler is now needed for
building. Effectively this means GCC 4.7 or higher, or Clang 3.3 or higher.

When cross-compiling for a target that doesn't have C++11 libraries, configure with
`./configure --enable-glibc-back-compat ... LDFLAGS=-static-libstdc++`.

For running the functional tests in `qa/rpc-tests`, Python3.4 or higher is now
required.


Linux ARM builds
----------------

Due to popular request, Linux ARM builds have been added to the uploaded
executables.

The following extra files can be found in the download directory or torrent:

- `verium-${VERSION}-arm-linux-gnueabihf.tar.gz`: Linux binaries targeting
  the 32-bit ARMv7-A architecture.
- `verium-${VERSION}-aarch64-linux-gnu.tar.gz`: Linux binaries targeting
  the 64-bit ARMv8-A architecture.

ARM builds are still experimental. If you have problems on a certain device or
Linux distribution combination please report them on the bug tracker, it may be
possible to resolve them. Note that the device you use must be (backward)
compatible with the architecture targeted by the binary that you use.
For example, a Raspberry Pi 2 Model B or Raspberry Pi 3 Model B (in its 32-bit
execution state) device, can run the 32-bit ARMv7-A targeted binary. However,
no model of Raspberry Pi 1 device can run either binary because they are all
ARMv6 architecture devices that are not compatible with ARMv7-A or ARMv8-A.

Note that Android is not considered ARM Linux in this context. The executables
are not expected to work out of the box on Android.

Reindexing changes
------------------

In earlier versions, reindexing did validation while reading through the block
files on disk. These two have now been split up, so that all blocks are known
before validation starts. This was necessary to make certain optimizations that
are available during normal synchronizations also available during reindexing.

The two phases are distinct in the Verium-Qt GUI. During the first one,
"Reindexing blocks on disk" is shown. During the second (slower) one,
"Processing blocks on disk" is shown.

It is possible to only redo validation now, without rebuilding the block index,
using the command line option `-reindex-chainstate` (in addition to
`-reindex` which does both). This new option is useful when the blocks on disk
are assumed to be fine, but the chainstate is still corrupted. It is also
useful for benchmarks.

Manual Pruning
--------------

Verium Vault has supported automatically pruning the blockchain since 1.3.0 Pruning
the blockchain allows for significant storage space savings as the vast majority of
the downloaded data can be discarded after processing so very little of it remains
on the disk.

Manual block pruning can now be enabled by setting `-prune=1`. Once that is set,
the RPC command `pruneblockchain` can be used to prune the blockchain up to the
specified height or timestamp.

`getinfo` Deprecated
--------------------

The `getinfo` RPC command has been deprecated. Each field in the RPC call
has been moved to another command's output with that command also giving
additional information that `getinfo` did not provide. The following table
shows where each field has been moved to:

|`getinfo` field   | Moved to                                  |
|------------------|-------------------------------------------|
`"version"`	   | `getnetworkinfo()["version"]`
`"protocolversion"`| `getnetworkinfo()["protocolversion"]`
`"walletversion"`  | `getwalletinfo()["walletversion"]`
`"balance"`	   | `getwalletinfo()["balance"]`
`"blocks"`	   | `getblockchaininfo()["blocks"]`
`"timeoffset"`	   | `getnetworkinfo()["timeoffset"]`
`"connections"`	   | `getnetworkinfo()["connections"]`
`"proxy"`	   | `getnetworkinfo()["networks"][0]["proxy"]`
`"difficulty"`	   | `getblockchaininfo()["difficulty"]`
`"testnet"`	   | `getblockchaininfo()["chain"] == "test"`
`"keypoololdest"`  | `getwalletinfo()["keypoololdest"]`
`"keypoolsize"`	   | `getwalletinfo()["keypoolsize"]`
`"unlocked_until"` | `getwalletinfo()["unlocked_until"]`
`"paytxfee"`	   | `getwalletinfo()["paytxfee"]`
`"relayfee"`	   | `getnetworkinfo()["relayfee"]`
`"errors"`	   | `getnetworkinfo()["warnings"]`

ZMQ On Windows
--------------

Previously the ZeroMQ notification system was unavailable on Windows
due to various issues with ZMQ. These have been fixed upstream and
now ZMQ can be used on Windows. Please see [this document](https://github.com/VeriumReserve/verium/blob/master/doc/zmq.md) for
help with using ZMQ in general.

Nested RPC Commands in Debug Console
------------------------------------

The ability to nest RPC commands has been added to the debug console. This
allows users to have the output of a command become the input to another
command without running the commands separately.

The nested RPC commands use bracket syntax (i.e. `getwalletinfo()`) and can
be nested (i.e. `getblock(getblockhash(1))`). Simple queries can be
done with square brackets where object values are accessed with either an
array index or a non-quoted string (i.e. `listunspent()[0][txid]`). Both
commas and spaces can be used to separate parameters in both the bracket syntax
and normal RPC command syntax.

Network Activity Toggle
-----------------------

A RPC command and GUI toggle have been added to enable or disable all p2p
network activity. The network status icon in the bottom right hand corner
is now the GUI toggle. Clicking the icon will either enable or disable all
p2p network activity. If network activity is disabled, the icon will
be grayed out with an X on top of it.

Additionally the `setnetworkactive` RPC command has been added which does
the same thing as the GUI icon. The command takes one boolean parameter,
`true` enables networking and `false` disables it.

Out-of-sync Modal Info Layer
----------------------------

When Verium is out-of-sync on startup, a semi-transparent information
layer will be shown over top of the normal display. This layer contains
details about the current sync progress and estimates the amount of time
remaining to finish syncing. This layer can also be hidden and subsequently
unhidden by clicking on the progress bar at the bottom of the window.

P2P connection management
--------------------------

- Peers manually added through the `-addnode` option or `addnode` RPC now have their own
  limit of eight connections which does not compete with other inbound or outbound
  connection usage and is not subject to the limitation imposed by the `-maxconnections`
  option.

- New connections to manually added peers are performed more quickly.


Support for JSON-RPC Named Arguments
------------------------------------

Commands sent over the JSON-RPC interface and through the `verium-cli` binary
can now use named arguments. This follows the [JSON-RPC specification](http://www.jsonrpc.org/specification)
for passing parameters by-name with an object.

`verium-cli` has been updated to support this by parsing `name=value` arguments
when the `-named` option is given.

Some examples:

    src/verium-cli -named help command="help"
    src/verium-cli -named getblockhash height=0
    src/verium-cli -named getblock blockhash=000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f
    src/verium-cli -named sendtoaddress address="(snip)" amount="1.0" subtractfeefromamount=true

The order of arguments doesn't matter in this case. Named arguments are also
useful to leave out arguments that should stay at their default value. The
rarely-used arguments `comment` and `comment_to` to `sendtoaddress`, for example, can
be left out. However, this is not yet implemented for many RPC calls, this is
expected to land in a later release.

The RPC server remains fully backwards compatible with positional arguments.

Sensitive Data Is No Longer Stored In Debug Console History
-----------------------------------------------------------

The debug console maintains a history of previously entered commands that can be
accessed by pressing the Up-arrow key so that users can easily reuse previously
entered commands. Commands which have sensitive information such as passphrases and
private keys will now have a `(...)` in place of the parameters when accessed through
the history.

Changed configuration options
-----------------------------

- `-includeconf=<file>` can be used to include additional configuration files.
  Only works inside the `verium.conf` file, not inside included files or from
  command-line. Multiple files may be included. Can be disabled from command-
  line via `-noincludeconf`. Note that multi-argument commands like
  `-includeconf` will override preceding `-noincludeconf`, i.e.
  ```
  noincludeconf=1
  includeconf=relative.conf
  ```

  as verium.conf will still include `relative.conf`.


External wallet files
---------------------

The `-wallet=<path>` option now accepts full paths instead of requiring wallets
to be located in the -walletdir directory.

Newly created wallet format
---------------------------

If `-wallet=<path>` is specified with a path that does not exist, it will now
create a wallet directory at the specified location (containing a wallet.dat
data file, a db.log file, and database/log.?????????? files) instead of just
creating a data file at the path and storing log files in the parent
directory. This should make backing up wallets more straightforward than
before because the specified wallet path can just be directly archived without
having to look in the parent directory for transaction log files.

For backwards compatibility, wallet paths that are names of existing data files
in the `-walletdir` directory will continue to be accepted and interpreted the
same as before.

Dynamic loading and creation of wallets
---------------------------------------

Previously, wallets could only be loaded or created at startup, by specifying `-wallet` parameters on the command line or in the verium.conf file. It is now possible to load, create and unload wallets dynamically at runtime:

- Existing wallets can be loaded by calling the `loadwallet` RPC. The wallet can be specified as file/directory basename (which must be located in the `walletdir` directory), or as an absolute path to a file/directory.
- New wallets can be created (and loaded) by calling the `createwallet` RPC. The provided name must not match a wallet file in the `walletdir` directory or the name of a wallet that is currently loaded.
- Loaded wallets can be unloaded by calling the `unloadwallet` RPC.

This feature is currently only available through the RPC interface.

Documentation
-------------

- A new short [document](https://github.com/VeriumReserve/verium/blob/master/doc/JSON-RPC-interface.md)
  about the JSON-RPC interface describes cases where the results of an
  RPC might contain inconsistencies between data sourced from different
  subsystems, such as wallet state and mempool state.  A note is added
  to the [REST interface documentation](https://github.com/VeriumReserve/verium/blob/master/doc/REST-interface.md)
  indicating that the same rules apply.

- Further information is added to the [JSON-RPC
  documentation](https://github.com/VeriumReserve/verium/blob/master/doc/JSON-RPC-interface.md)
  about how to secure this interface.

- A new [document](https://github.com/VeriumReserve/verium/blob/master/doc/verium-conf.md)
  about the `verium.conf` file describes how to use it to configure
  Verium.

systemd init file
-----------------

The systemd init file (`contrib/init/veriumd.service`) has been changed
to use `/var/lib/veriumd` as the data directory instead of
`~verium/.verium`. This change makes Verium more consistent with
other services, and makes the systemd init config more consistent with
existing Upstart and OpenRC configs.

The configuration, PID, and data directories are now completely managed
by systemd, which will take care of their creation, permissions, etc.
See [`systemd.exec(5)`](https://www.freedesktop.org/software/systemd/man/systemd.exec.html#RuntimeDirectory=)
for more details.

When using the provided init files under `contrib/init`, overriding the
`datadir` option in `/etc/verium/verium.conf` will have no effect.
This is because the command line arguments specified in the init files
take precedence over the options specified in
`/etc/verium/verium.conf`.

New GUI
-------------

The Verium Vault User interface have been totally rewrote. The main goal
is to improve the user experience and to make it compatible with more
screen resolution.

Verium Core
-------------

Verium 1.2 was based on Bitcoin Core 0.8. The Verium Vault 1.3 is based on
Bitcoin Core 0.19. A big part of the previous listed change are part of it.
In the future we will try to stick more to the Bitcoin Core Version in
order to make most verium apps compatible with Verium Vault. Due to that
huge version bump it was impossible to list all the change in that Changelog

1.3.0 Change log
=======================

RPC:

- New notion of 'conflicted' transactions, reported as confirmations: -1
- 'listreceivedbyaddress' now provides tx ids
- Add raw transaction hex to 'gettransaction' output
- Updated help and tests for 'getreceivedby(account|address)'
- In 'getblock', accept 2nd 'verbose' parameter, similar to getrawtransaction,
  but defaulting to 1 for backward compatibility
- Add 'verifychain', to verify chain database at runtime
- Add 'dumpwallet' and 'importwallet' RPCs
- 'keypoolrefill' gains optional size parameter
- Add 'getbestblockhash', to return tip of best chain
- Add 'chainwork' (the total work done by all blocks since the genesis block)
  to 'getblock' output
- Make RPC password resistant to timing attacks
- Clarify help messages and add examples
- Add 'getrawchangeaddress' call for raw transaction change destinations
- Reject insanely high fees by default in 'sendrawtransaction'
- Add RPC call 'decodescript' to decode a hex-encoded transaction script
- Make 'validateaddress' provide redeemScript
- Add 'getnetworkhashps' to get the calculated network hashrate
- New RPC 'ping' command to request ping, new 'pingtime' and 'pingwait' fields
  in 'getpeerinfo' output
- Adding new 'addrlocal' field to 'getpeerinfo' output
- Add verbose boolean to 'getrawmempool'
- Add rpc command 'getunconfirmedbalance' to obtain total unconfirmed balance
- Explicitly ensure that wallet is unlocked in `importprivkey`
- Add check for valid keys in `importprivkey`
- Add getwalletinfo, getblockchaininfo and getnetworkinfo calls
- Remove getinfo call
- Fix RPC related shutdown hangs and leaks
- Always show syncnode in getpeerinfo
- sendrawtransaction: report the reject code and reason, and make it possible to re-send transactions that are already in the mempool
- getmininginfo show right genproclimit
- Avoid a segfault on getblock if it can't read a block from disk
- Add paranoid return value checks in base58
- Disable SSL for the RPC client and server. (use a proxypass if needed)
- Sanitize command strings before logging them.
- Support IPv6 lookup in verium-cli even when IPv6 only bound on localhost
- Fix addnode "onetry": Connect with OpenNetworkConnection
- Add "chain" to getmininginfo, improve help in getblockchaininfo
- Add nLocalServices info to RPC getinfo
- Remove getwork() RPC call
- Prevent easy RPC memory exhaustion attack
- Added argument to getbalance to include watchonly addresses and fixed errors in balance calculation
- Added argument to listaccounts to include watchonly addresses
- Showing 'involvesWatchonly' property for transactions returned by 'listtransactions' and 'listsinceblock'. It is only appended when the transaction involves a watchonly address
- Added argument to listtransactions and listsinceblock to include watchonly addresses
- added includeWatchonly argument to 'gettransaction' because it affects balance calculation
- added includedWatchonly argument to listreceivedbyaddress/...account
- getrawchangeaddress: fail when keypool exhausted and wallet locked
- getblocktemplate: longpolling support
- Add peerid to getpeerinfo to allow correlation with the logs
- Add vout to ListTransactions output
- Remove size limit in RPC client, keep it in server
- Categorize rpc help overview
- Closely track mempool byte total. Add "getmempoolinfo" RPC
- Add detailed network info to getnetworkinfo RPC
- Don't reveal whether password is <20 or >20 characters in RPC
- Compute number of confirmations of a block from block height
- getnetworkinfo: export local node's client sub-version string
- SanitizeString: allow '(' and ')'
- Fixed setaccount accepting foreign address
- update getnetworkinfo help with subversion
- RPC additions after headers-first
- Fix leveldb iterator leak, and flush before gettxoutsetinfo
- Add "warmup mode" for RPC server
- Add unauthenticated HTTP REST interface to public blockchain data
- signrawtransaction: validate private key
- ...

Command-line options:

- New option: -zapwallettxes to rebuild the wallet's transaction information
- Rename option '-tor' to '-onion' to better reflect what it does
- Add '-disablewallet' mode to let veriumd run entirely without wallet (when
  built with wallet)
- Update default '-rpcsslciphers' to include TLSv1.2 & TLSv1.3
- make '-logtimestamps' default on and rework help-message
- RPC client option: '-rpcwait', to wait for server start
- Remove '-logtodebugger'
- Show error message if ReadConfigFile fails
- Make -proxy set all network types, avoiding a connect leak.
- Use netmasks instead of wildcards for IP address matching
- Add -rpcbind option to allow binding RPC port on a specific interface
- Add -version option to get just the version
- rework help messages for fee-related options
- Clarify error message when invalid -rpcallowip
- "-datadir" is now allowed in config files
- Add option -sysperms to disable 077 umask (create new files with system default umask)
- Add "verium-tx" command line utility and supporting modules
- Make -reindex cope with out-of-order blocks
- Skip reindexed blocks individually
- Change -genproclimit default to 1
- Remove -printblock, -printblocktree, and -printblockindex
- alias -h for --help
- ...

Block-chain handling and storage:

- Update leveldb to 1.17
- Check for correct genesis (prevent cases where a datadir from the wrong
  network is accidentally loaded)
- Allow txindex to be removed and add a reindex dialog
- Log aborted block database rebuilds
- Store orphan blocks in serialized form, to save memory
- Limit the number of orphan blocks in memory to 750
- Fix non-standard disconnected transactions causing mempool orphans
- ...

Block and transaction handling:
- ProcessGetData(): abort if a block file is missing from disk
- LoadBlockIndexDB(): Require block db reindex if any blk*.dat files are missing
- Push cs_mains down in ProcessBlock
- Avoid undefined behavior using CFlatData in CScript serialization
- Relax IsStandard rules for pay-to-script-hash transactions
- Add a skiplist to the CBlockIndex structure
- Use unordered_map for CCoinsViewCache with salted hash (optimization)
- Do not flush the cache after every block outside of IBD (optimization)
- Bugfix: make CCoinsViewMemPool support pruned entries in underlying cache
- Only remove actualy failed blocks from setBlockIndexValid
- Rework block processing benchmark code
- Only keep setBlockIndexValid entries that are possible improvements
- Reduce maximum coinscache size during verification (reduce memory usage)
- Reject transactions with excessive numbers of sigops
- Allow BatchWrite to destroy its input, reducing copying (optimization)
- Bypass reloading blocks from disk (optimization)
- Perform CVerifyDB on pcoinsdbview instead of pcoinsTip (reduce memory usage)
- Avoid copying undo data (optimization)
- Headers-first synchronization
- Fix rebuild-chainstate feature and improve its performance
- Fix large reorgs
- Keep information about all block files in memory
- Abstract context-dependent block checking from acceptance
- Fixed mempool sync after sending a transaction
- Improve chainstate/blockindex disk writing policy
- Introduce separate flushing modes
- Add a locking mechanism to IsInitialBlockDownload to ensure it never goes from false to true
- Remove coinbase-dependant transactions during reorg
- Remove txn which are invalidated by coinbase maturity during reorg
- Check against MANDATORY flags prior to accepting to mempool
- Reject headers that build on an invalid parent
- Bugfix: only track UTXO modification after lookup
- ...

Wallet:

- Bug fixes and new regression tests to correctly compute
  the balance of wallets containing double-spent (or mutated) transactions
- Store key creation time. Calculate whole-wallet birthday.
- Optimize rescan to skip blocks prior to birthday
- Let user select wallet file with -wallet=foo.dat
- Improve wallet load time
- Don't create empty transactions when reading a corrupted wallet
- Fix rescan to start from beginning after importprivkey
- ...

Mining:

- 'getblocktemplate' does not require a key to create a block template
- ...

Protocol and network:
- New 'reject' P2P message (BIP 0061, see
  https://gist.github.com/gavinandresen/7079034 for draft)
- Relay OP_RETURN data TxOut as standard transaction type
- Remove CENT-output free transaction rule when relaying
- Lower maximum size for free transaction creation
- Send multiple inv messages if mempool.size > MAX_INV_SZ
- Split MIN_PROTO_VERSION into INIT_PROTO_VERSION and MIN_PEER_PROTO_VERSION
- Do not treat fFromMe transaction differently when broadcasting
- Process received messages one at a time without sleeping between messages
- Improve logging of failed connections
- Add some additional logging to give extra network insight
- Per-peer block download tracking and stalled download detection
- Prevent socket leak in ThreadSocketHandler and correct some proxy related socket leaks
- Use pnode->nLastRecv as sync score (was the wrong way around)
- Remove a useless millisleep in socket handler
- Stricter memory limits on CNode
- Better orphan transaction handling
- Limit the number of new addressses to accumulate
- Do not trigger a DoS ban if SCRIPT_VERIFY_NULLDUMMY fails
- Ping automatically every 2 minutes (unconditionally)
- Allocate receive buffers in on the fly
- Display unknown commands received
- Track peers' available blocks
- Use async name resolving to improve net thread responsiveness
- Use pong receive time rather than processing time
- remove SOCKS4 support from core and GUI, use SOCKS5
- Send rejects and apply DoS scoring for errors in direct block validation
- Introduce whitelisted peers
- prevent SOCKET leak in BindListenPort()
- Add built-in seeds for .onion
- Allow -onlynet=onion to be used
- addrman: Do not propagate obviously poor addresses onto the network
- netbase: Make SOCKS5 negotiation interruptible
- Remove tx from AlreadyAskedFor list once we receive it, not when we process it
- Avoid reject message feedback loops
- Separate protocol versioning from clientversion
- Introduce preferred download peers
- Do not use third party services for IP detection
- Limit the number of new addressses to accumulate
- Regard connection failures as attempt for addrman
- Introduce 10 minute block download timeout
- Scale up addrman (countermeasure 6 against eclipse attacks)
- Make addrman's bucket placement deterministic
- Don't trickle for whitelisted nodes
- ...

Wallet:

- Make GetAvailableCredit run GetHash() only once per transaction (performance improvement)
- Fix importwallet nTimeFirstKey (trigger necessary rescans)
- Log BerkeleyDB version at startup
- Sanity checks for estimates
- Add support for watch-only addresses
- Use script matching rather than destination matching for watch-only
- Dont run full check every time we decrypt wallet
- fix a possible memory leak in CWalletDB::Recover
- fix possible memory leaks in CWallet::EncryptWallet
- ...

Validation:

- Log reason for non-standard transaction rejection
- Prune provably-unspendable outputs, and adapt consistency check for it.
- Fix multi-block reorg transaction resurrection
- Reject non-canonically-encoded serialization sizes
- Reject dust amounts during validation
- Accept nLockTime transactions that finalize in the next block
- Fail immediately on an empty signature
- Do merkle root and txid duplicates check simultaneously
- When transaction outputs exceed inputs, show the offending amounts so as to aid debugging
- Print input index when signature validation fails, to aid debugging
- Make STRICTENC invalid pubkeys fail the script rather than the opcode
- Report script evaluation failures in log and reject messages
- Improve robustness of DER recoding code
- ...

Build system:

- Switch to autotools-based build system
- Build without wallet by passing `--disable-wallet` to configure, this
  removes the BerkeleyDB dependency
- Windows 64-bit build support
- Enable full GCC Stack-smashing protection for all OSes
- ...

GUI:

- Switch to Qt 5.7.0 for Windows build
- Redesign the full UX

Miscellaneous:

- Add Linux script (contrib/qos/tc.sh) to limit outgoing bandwidth
  generation with 'setgenerate' RPC.
- Add separate verium-cli client
- Replace non-threadsafe C functions
- Add missing cs_main and wallet locks
- Avoid exception at startup when system locale not recognized
- Add a script to fetch and postprocess translations
- Fix boost detection in build system on some platforms
- Update all dependency version
- libsecp256k1 integration
- Show nodeid instead of addresses in log (for anonymity) unless otherwise requested
- Enable paranoid corruption checks in LevelDB >= 1.16
- contrib: Added systemd .service file in order to help distributions integrate veriumd
- Update coding style and add .clang-format
- Changed LevelDB cursors to use scoped pointers to ensure destruction when going out of scope
- Use explicit fflush() instead of setvbuf()
- contrib: Add init scripts and docs for Upstart and OpenRC
- Add warning about the merkle-tree algorithm duplicate txid flaw
- Add bash-completion for 1.3.0
- Catch LevelDB errors during flush
- Fix CScriptID(const CScript& in) in empty script case
- Move from Bitcoin Core 0.8 code base to 0.19

Credits
--------

Thanks to everyone who contributed to this release:
- Bitcoin Core Team
- Sinetek
- hd4r
- Douglas Pike (effectsToCause)
- Matthieu Derasse (mderasse)

And thanks to the community to keep following us and all their work