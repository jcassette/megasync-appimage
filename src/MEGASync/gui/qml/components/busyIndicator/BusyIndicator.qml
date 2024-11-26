import QtQuick 2.15
import QtQuick.Controls 2.15 as Qml
import QtGraphicalEffects 1.15

import common 1.0

import components.images 1.0

Qml.BusyIndicator {
    id: root

    property alias color: iconImage.color
    property alias imageSource: iconImage.source
    property alias imageSize: iconImage.sourceSize

    width: imageSize.width
    height: imageSize.height

    contentItem: SvgImage {
        id: iconImage

        source: Images.loader
        anchors.fill: parent
        anchors.centerIn: parent
        visible: root.visible
    }

    ConicalGradient {
        anchors.fill: iconImage
        source: iconImage
        angle: 120
        gradient: Gradient {
            GradientStop {  position: 0.0; color: "#303233"; }
            GradientStop { position: 1.0; color: "transparent"; }
        }
    }

    RotationAnimator on rotation {
        running: root.visible
        loops: Animation.Infinite
        duration: 2000
        from: 0 ; to: 360
    }

}
