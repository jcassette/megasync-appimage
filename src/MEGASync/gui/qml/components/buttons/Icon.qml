import QtQuick 2.15

import common 1.0

QtObject {
    id: root

    enum Position {
        LEFT = 0,
        RIGHT,
        BOTH
    }

    property color colorEnabled: ColorTheme.textInverseAccent
    property color colorDisabled: ColorTheme.textDisabled
    property color colorHovered: ColorTheme.textInverseAccent
    property color colorPressed: ColorTheme.textInverseAccent
    property string source
    property int position: Icon.Position.RIGHT
    property int busyIndicatorPosition: Icon.Position.RIGHT
    property bool busyIndicatorVisible: false
}

