# CREDITS

Here is the list of the third party libraries used by MEGA Desktop Application.
We are grateful and thankful for their efforts and the opportunity to rely and
extend on their existing body of work.

## Qt SDK
http://www.qt.io/ © 2023 The Qt Company Ltd.

### Description
Qt is the cross-platform framework for C++ GUI development.

### Usage
Qt is used by MEGA Desktop to get cross-platform compatibility and GUI
functionality across the supported desktop operating systems.

### Licence 
Dual licence: LGPL 3

https://www.gnu.org/licenses/lgpl-3.0.html

http://www.qt.io/licensing/

--------------------------------------------------------------------

#### Google Breakpad
https://chromium.googlesource.com/breakpad/breakpad/

Copyright (c) 2006, Google Inc.

###### Description:
Breakpad is a set of client and server components which implement a 
crash-reporting system.

###### Usage:
MEGAsync uses this library to capture crashes on Windows and OS X

###### License:
The BSD 3-Clause License

https://chromium.googlesource.com/breakpad/breakpad/+/main/LICENSE

--------------------------------------------------------------------

#### Bitcoin Core
https://github.com/bitcoin/bitcoin

Copyright (c) 2011-2013 The Bitcoin Core developers

###### Description:
Bitcoin is an experimental new digital currency that enables instant 
payments to anyone, anywhere in the world. Bitcoin uses peer-to-peer 
technology to operate with no central authority: managing transactions 
and issuing money are carried out collectively by the network. Bitcoin Core 
is the name of open source software which enables the use of this currency.

###### Usage:
MEGAsync uses some files in that repository to show desktop notifications 
on OS X and Linux. Specifically, MEGAsync uses these files:
- notificator.cpp 
- notificator.h 
- macnotificationhandler.mm 
- macnotificationhandler.h

from this folder: https://github.com/bitcoin/bitcoin/blob/master/src/qt/

###### License: 
MIT/X11 software license

http://www.opensource.org/licenses/mit-license.php

--------------------------------------------------------------------

#### C++ Windows Shell context menu handler (CppShellExtContextMenuHandler)
https://code.msdn.microsoft.com/windowsapps/CppShellExtContextMenuHandl-410a709a

Copyright (c) Microsoft Corporation.

###### Description:
Example code from Microsoft for the creation of a context menu for
Windows Explorer

###### Usage:
This example was the base for the implementation of the shell extension of
MEGAsync on Windows (MEGAShellExt project in this repository)

###### License:
Microsoft Public License.

https://opensource.org/licenses/MS-PL

--------------------------------------------------------------------

#### QtLockedFile
https://github.com/qtproject/qt-solutions/tree/master/qtlockedfile/src

Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).

###### Description:
Provides locking between processes using a file.

###### Usage:
MEGAsync uses this library to detect if it's already running and avoid to have
several instances running at once.

###### License: 
The BSD 3-Clause License

--------------------------------------------------------------------

#### NSPopover+MISSINGBackgroundView
Copyright (c) 2015 Valentin Shergin

A very tiny library that allows to access (and customize) background view of NSPopover.

https://github.com/shergin/NSPopover-MISSINGBackgroundView

License: MIT

https://github.com/shergin/NSPopover-MISSINGBackgroundView/blob/master/LICENSE

--------------------------------------------------------------------

#### QtWaitingSpinner
Original Work Copyright (c) 2012-2015 Alexander Turkin

QtWaitingSpinner is a highly configurable, custom Qt widget for showing spinner icons in Qt applications.

https://github.com/snowwlex/QtWaitingSpinner

License: MIT

https://github.com/snowwlex/QtWaitingSpinner/blob/master/LICENSE

--------------------------------------------------------------------

#### MEGA C++ SDK
https://github.com/meganz/sdk

(c) 2013-2016 by Mega Limited, Auckland, New Zealand

###### Description:
MEGA --- The Privacy Company --- is a Secure Cloud Storage provider that protects 
your data thanks to end-to-end encryption. We call it User Controlled Encryption, 
or UCE, and all our clients automatically manage it.

All files stored on MEGA are encrypted. All data transfers from and to MEGA 
are encrypted. And while most cloud storage providers can and do claim the same, 
MEGA is different – unlike the industry norm where the cloud storage provider 
holds the decryption key, with MEGA, you control the encryption, you hold the keys, 
and you decide who you grant or deny access to your files.

This SDK brings you all the power of our client applications and let you create 
your own or analyze the security of our products.

###### Usage:
MEGAsync uses the MEGA C++ SDK to get all functionality that requires access
to MEGA servers.

###### License:
Simplified (2-clause) BSD License.

https://github.com/meganz/sdk/blob/master/LICENSE

--------------------------------------------------------------------

#### Dependencies of the MEGA C++ SDK
Due to the usage of the MEGA C++ SDK, MEGAsync requires some additional 
libraries. Here is a brief description of all of them:

#### c-ares:
Copyright 1998 by the Massachusetts Institute of Technology.

c-ares is a C library for asynchronous DNS requests (including name resolves)

http://c-ares.haxx.se/

License: MIT license

http://c-ares.haxx.se/license.html

#### sodium:
Copyright (c) 2013-2023, Frank Denis <j at pureftpd dot org>

Sodium is a modern, portable, easy to use crypto library.

https://libsodium.org/

License: ISC License

https://github.com/jedisct1/libsodium/blob/master/LICENSE

#### SQLite
SQLite is an in-process library that implements a self-contained, serverless, zero-configuration, transactional SQL database engine.

http://www.sqlite.org/

License: Public Domain

http://www.sqlite.org/copyright.html

#### zlib:
copyright (c) 1995-2022 Jean-loup Gailly and Mark Adler.

zlib is a general purpose data compression libray.

http://zlib.net/

License: zlib license.

http://zlib.net/zlib_license.html

#### freeimage
Copyright (c) 2003-2023 by FreeImage. All rights reserved.

FreeImage is an Open Source library project for developers who would like
 to support popular graphics image formats like PNG, BMP, JPEG, TIFF and
 others as needed by today's multimedia applications.

This software uses the FreeImage open source image library.
https://freeimage.sourceforge.io/ for details.

License: FreeImage Public License - Version 1.0.

https://freeimage.sourceforge.io/freeimage-license.txt

#### libmediainfo:
Copyright (c) MediaArea.net SARL. All rights reserved.

MediaInfo(Lib) is a convenient unified display of the most relevant technical and tag data for video and audio files.

License: BSD 2-Clause License

https://github.com/MediaArea/MediaInfoLib/blob/master/LICENSE

#### libcurl
Copyright (c) 1996 - 2023, Daniel Stenberg, <daniel@haxx.se>, et al.

The multiprotocol file transfer library

https://curl.se/libcurl/

License:  MIT/X derivate license

https://curl.se/docs/copyright.html

#### Crypto++
Copyright (c) 1995-2019 by Wei Dai. (for the compilation) and public domain (for individual files)

Crypto++ Library is a free C++ class library of cryptographic schemes.

https://www.cryptopp.com/

License: Crypto++ Library is copyrighted as a compilation and (as of version 5.6.2) 
licensed under the Boost Software License 1.0, while the individual files in 
the compilation are all public domain.

#### OpenSSL
Copyright (c) 1998-2022 The OpenSSL Project. All rights reserved.

A toolkit implementing SSL v2/v3 and TLS protocols with full-strength cryptography world-wide.

https://www.openssl.org/

License: OpenSSL License

https://github.com/openssl/openssl/blob/master/LICENSE

#### libuv
Copyright (c) 2015-present libuv project contributors.
Copyright Joyent, Inc. and other Node contributors. All rights reserved.

libuv is a multi-platform support library with a focus on asynchronous I/O.

https://libuv.org/

License: MIT

https://github.com/libuv/libuv/blob/v1.x/LICENSE

#### LibRaw
Copyright © 2008-2023 LibRaw LLC (info@libraw.org)

LibRaw is a library for reading RAW files obtained from digital photo cameras (CRW/CR2, NEF, RAF, DNG, and others).

https://www.libraw.org/

License: LGPL 2.1 or CDDL 1.0

https://github.com/LibRaw/LibRaw/blob/master/LICENSE.LGPL
https://github.com/LibRaw/LibRaw/blob/master/LICENSE.CDDL

#### PDFium
Copyright 2014 PDFium Authors. All rights reserved.

PDF generation and rendering library.

https://pdfium.googlesource.com/pdfium/

License: BSD 3-clause

https://pdfium.googlesource.com/pdfium/+/main/LICENSE

#### FFmpeg
This software uses code of <a href=http://ffmpeg.org>FFmpeg</a> licensed under the <a href=http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html>LGPLv2.1</a> and its source can be downloaded <a href=https://www.ffmpeg.org/download.html>here</a>

FFmpeg is a library for working with many video formats, we use it to generate thumbnails and preview images for relevant files.

https://www.ffmpeg.org/about.html

License: LGPLv2.1

http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html

--------------------------------------------------------------------

#### Source Sans Pro fonts
https://github.com/adobe-fonts/source-sans-pro

Copyright 2010, 2012, 2014 Adobe Systems Incorporated (http://www.adobe.com/), with Reserved Font Name 'Source'. All Rights Reserved. Source is a trademark of Adobe Systems Incorporated in the United States and/or other countries.

###### Description:
Set of OpenType fonts that have been designed to work well in user interface (UI) environments.

###### Usage:
MEGAsync uses as default application font. 

###### License: 
SIL OPEN FONT LICENSE Version 1.1 - 26 February 2007

https://github.com/adobe-fonts/source-sans-pro/blob/master/LICENSE.txt

--------------------------------------------------------------------
