import QtQuick 2.15

import common 1.0

import components.texts 1.0 as Texts
import components.images 1.0

Row {
    id: root

    readonly property int iconWidth: 16

    property alias text: condition.text

    property bool checked: false

    width: parent.width
    spacing: iconWidth / 2

    SvgImage {
        id: image

        color: checked ? ColorTheme.indicatorGreen : ColorTheme.textSecondary
        source: checked ? Images.check : Images.smallCircle
        sourceSize: Qt.size(iconWidth, iconWidth)
    }

    Texts.SecondaryText {
        id: condition

        width: parent.width - iconWidth
        font.strikeout: checked
    }

}

