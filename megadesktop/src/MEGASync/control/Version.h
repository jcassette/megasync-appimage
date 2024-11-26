#ifndef VERSION_H
#define VERSION_H

#define VER_FILEVERSION 5, 6, 0, 0
#define VER_FILEVERSION_CODE 50600
#define VER_PRODUCTVERSION 5, 6, 0, 0
// Update scripts relying on this value if you move it
#define VER_PRODUCTVERSION_STR "5.6.0.0\0"

#define VER_BUILD_ID 1

#define VER_COMPANYNAME_STR         "Mega Limited\0"
#define VER_FILEDESCRIPTION_STR     "MEGAsync\0"
#define VER_INTERNALNAME_STR        "MEGAsync.exe\0"
#define VER_LEGALCOPYRIGHT_STR      "Mega Limited 2024\0"
#define VER_LEGALTRADEMARKS1_STR    "All Rights Reserved"
#define VER_ORIGINALFILENAME_STR    "MEGAsync.exe\0"
#define VER_PRODUCTNAME_STR         "MEGAsync\0"

#define VER_SDK_ID "6073288"

/* Update scrips relying on this value if you move it
Format: 1 item by line, starting from line following the #define
#define VER_CHANGES_NOTES QT_TRANSLATE_NOOP("Preferences",
"- item 1\n"
"- item 2\n\"
 [...]
"- item n\n"
)*/
#define VER_CHANGES_NOTES \
QT_TRANSLATE_NOOP("Preferences", \
                  "- We’ve improved the automatic and manual resolution of sync issues.\n" \
                  "- The MEGA account interface and the account upgrade process have both been " \
                  "revamped and enhanced.\n" \
                  "- Other bugs have been fixed and numerous improvements made.\n")

#endif // VERSION_H
