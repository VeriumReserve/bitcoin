#!/usr/bin/env bash

export LC_ALL=C
TOPDIR=${TOPDIR:-$(git rev-parse --show-toplevel)}
BUILDDIR=${BUILDDIR:-$TOPDIR}

BINDIR=${BINDIR:-$BUILDDIR/src}
MANDIR=${MANDIR:-$TOPDIR/doc/man}

VERIUMD=${VERIUMD:-$BINDIR/veriumd}
VERIUMCLI=${VERIUMCLI:-$BINDIR/verium-cli}
VERIUMTX=${VERIUMTX:-$BINDIR/verium-tx}
WALLET_TOOL=${WALLET_TOOL:-$BINDIR/verium-wallet}
VERIUMQT=${VERIUMQT:-$BINDIR/qt/verium-qt}

[ ! -x $VERIUMD ] && echo "$VERIUMD not found or not executable." && exit 1

# The autodetected version git tag can screw up manpage output a little bit
read -r -a BTCVER <<< "$($VERIUMCLI --version | head -n1 | awk -F'[ -]' '{ print $5, $6 }')"

# Create a footer file with copyright content.
# This gets autodetected fine for veriumd if --version-string is not set,
# but has different outcomes for verium-qt and verium-cli.
echo "[COPYRIGHT]" > footer.h2m
$VERIUMD --version | sed -n '1!p' >> footer.h2m

for cmd in $VERIUMD $VERIUMCLI $VERIUMTX $WALLET_TOOL $VERIUMQT; do
  cmdname="${cmd##*/}"
  help2man -N --version-string=${BTCVER[0]} --include=footer.h2m -o ${MANDIR}/${cmdname}.1 ${cmd}
  sed -i "s/\\\-${BTCVER[1]}//g" ${MANDIR}/${cmdname}.1
done

rm -f footer.h2m
