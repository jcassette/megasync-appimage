import QtQuick 2.15

import common 1.0

import GuestQmlDialog 1.0

GuestQmlDialog {
    id: window

    width: 400
    height: 560
    color: "transparent"

    onVisibleChanged:  {
        if(visible) {
            accountStatusControllerAccess.whyAmIBlocked();
        }
    }

    onGuestActiveChanged: (active) => {
        if (!active) {
            fadeOut.start();
        }
    }

    Rectangle {
        id: borderItem

        width: window.width
        height: window.height
        color: ColorTheme.surface1
        radius: 10
        border.color: "#1F000000"
        border.width: 1
    }

    GuestFlow {
        id: guestFlow

        width: window.width
        height: window.height
    }

    PropertyAnimation {
        id: fadeOut

        target: window
        property: "opacity"
        to: 0
        duration: 100

        onRunningChanged: {
            if (!running) {
                window.hide();
                window.opacity = 1;
            }
        }
    }
}
