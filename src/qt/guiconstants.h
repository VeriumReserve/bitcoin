// Copyright (c) 2011-2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_GUICONSTANTS_H
#define BITCOIN_QT_GUICONSTANTS_H

#include <QString>
#include <QTranslator>
#include <QFont>

/* Milliseconds between model updates */
static const int MODEL_UPDATE_DELAY = 250;

/* AskPassphraseDialog -- Maximum passphrase length */
static const int MAX_PASSPHRASE_SIZE = 1024;

/* BitcoinGUI -- Size of icons in status bar */
static const int STATUSBAR_ICONSIZE = 16;

static const bool DEFAULT_SPLASHSCREEN = true;

/* Invalid field background style */
#define STYLE_INVALID "background:#FF8080"

/* Transaction list -- unconfirmed transaction */
#define COLOR_UNCONFIRMED QColor(128, 128, 128)
/* Transaction list -- negative amount */
#define COLOR_NEGATIVE QColor(255, 0, 0)
/* Transaction list -- bare address (without label) */
#define COLOR_BAREADDRESS QColor(140, 140, 140)
/* Transaction list -- TX status decoration - open until date */
#define COLOR_TX_STATUS_OPENUNTILDATE QColor(64, 64, 255)
/* Transaction list -- TX status decoration - danger, tx needs attention */
#define COLOR_TX_STATUS_DANGER QColor(200, 100, 100)
/* Transaction list -- TX status decoration - default color */
#define COLOR_BLACK QColor(0, 0, 0)

/* Tooltips longer than this (in characters) are converted into rich text,
   so that they can be word-wrapped.
 */
static const int TOOLTIP_WRAP_THRESHOLD = 80;

/* Maximum allowed URI length */
static const int MAX_URI_LENGTH = 255;

/* QRCodeDialog -- size of exported QR Code image */
#define QR_IMAGE_SIZE 300

/* Number of frames in spinner animation */
#define SPINNER_FRAMES 36

#define QAPP_ORG_NAME "Bitcoin"
#define QAPP_ORG_DOMAIN "bitcoin.org"
#define QAPP_APP_NAME_DEFAULT "Bitcoin-Qt"
#define QAPP_APP_NAME_TESTNET "Bitcoin-Qt-testnet"

/************
 ** Verium **/
// These magic values are taken from the old client implementation.
// Adjust them as needed.
const int TOOLBAR_WIDTH = 120;
const int TOOLBAR_ICON_WIDTH = TOOLBAR_WIDTH;
const int TOOLBAR_ICON_HEIGHT = 48;
const int HEADER_WIDTH = 900;
const int HEADER_HEIGHT = 0;
const int BUTTON_WIDTH = 140;
const int BUTTON_HEIGHT = 27;

/* Custom colors / fonts */
#define STR_COLOR QString("#586a7a")
#define STR_COLOR_LT QString("#46505a")
#define STR_COLOR_HOVER QString("#46505a")
#define STR_COLOR_TTBG QString("#86909C")
#define STR_FONT_COLOR QString("#46505a")

#ifdef Q_OS_MAC
static const QFont qFontSmallest("Lato", 9, QFont::Normal, false);
static const QFont qFontSmaller("Lato", 11, QFont::Normal, false);
static const QFont qFontSmall("Lato", 13, QFont::Normal, false);
static const QFont qFont("Lato", 15, QFont::Normal, false);
static const QFont qFontLarge("Lato", 17, QFont::Normal, false);
static const QFont qFontLarger("Lato", 19, QFont::Normal, false);
static const QFont qFontSmallerBold("Lato", 11, QFont::Bold, false);
static const QFont qFontSmallBold("Lato", 13, QFont::Bold, false);
static const QFont qFontBold("Lato", 15, QFont::Bold, false);
static const QFont qFontLargeBold("Lato", 17, QFont::Bold, false);
static const QFont qFontLargerBold("Lato", 19, QFont::Bold, false);
#else
static const QFont qFontSmallest("Lato", 8, QFont::Normal, false);
static const QFont qFontSmaller("Lato", 9, QFont::Normal, false);
static const QFont qFontSmall("Lato", 10, QFont::Normal, false);
static const QFont qFont("Lato", 11, QFont::Normal, false);
static const QFont qFontLarge("Lato", 12, QFont::Normal, false);
static const QFont qFontLarger("Lato", 14, QFont::Normal, false);
static const QFont qFontSmallerBold("Lato", 9, QFont::Bold, false);
static const QFont qFontSmallBold("Lato", 10, QFont::Bold, false);
static const QFont qFontBold("Lato", 11, QFont::Bold, false);
static const QFont qFontLargeBold("Lato", 12, QFont::Bold, false);
static const QFont qFontLargerBold("Lato", 14, QFont::Bold, false);
#endif

 /** !VERIUM **/

#endif // BITCOIN_QT_GUICONSTANTS_H
