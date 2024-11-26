import QtQuick 2.15

import common 1.0

import components.texts 1.0 as Texts

QtObject {
    id: root

    property bool borderLess: false

    // Medium sizes
    property int horizontalPadding: borderLess ? 8 : 16
    property int verticalPadding: horizontalPadding/2
    property int spacing: 8
    property real radius: 6
    property int maskBorderRadius: 8
    property real focusBorderRadius: 10.5
    property int focusBorderWidth: Constants.focusBorderWidth
    property int borderWidth: 2
    property int iconWidth: 16
    property size iconSize: Qt.size(iconWidth, iconWidth)
    property int textFontSize: Texts.Text.Size.MEDIUM
    property int textLineHeight: 20
    property int horizontalAlignWidth: focusBorderWidth + horizontalPadding
    property int verticalAlignWidth: focusBorderWidth + verticalPadding

}
