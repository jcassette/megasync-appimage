import QtQuick 2.15

import common 1.0

import components.texts 1.0 as Texts

QtObject {
    id: root

    // Medium sizes
    property int padding: 8
    property int height: 36
    property int titleBottomMargin: 4
    property int iconMargin: 13
    property int iconWidth: 16
    property size iconSize: Qt.size(iconWidth, iconWidth)
    property int iconTextSeparation: 6
    property int focusBorderRadius: 12
    property int focusBorderWidth: Constants.focusBorderWidth
    property int borderRadius: 8
    property int borderWidth: 1
    property int hintTextSize: Texts.Text.Size.NORMAL
    property int titleSpacing: 1

}
