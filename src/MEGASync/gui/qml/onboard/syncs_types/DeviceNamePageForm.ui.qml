import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15

import common 1.0

import components.textFields 1.0
import components.images 1.0
import components.pages 1.0

import onboard 1.0

FooterButtonsPage {
    id: root

    property alias deviceNameTextField: deviceNameTextFieldComp

    footerButtons.rightSecondary.visible: false
    footerButtons.leftPrimary.visible: false

    ColumnLayout {
        id: mainLayout

        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }
        spacing: 12

        HeaderTexts {
            id: headerItem

            Layout.preferredWidth: parent.width
            title: OnboardingStrings.deviceNameTitle
            description: OnboardingStrings.deviceNameDescription
        }

        SvgImage {
            id: image

            Layout.topMargin: 20
            source: Images.pcMega
            sourceSize: Qt.size(48, 48)
            color: ColorTheme.textPrimary
        }

        TextField {
            id: deviceNameTextFieldComp

            Layout.leftMargin: Constants.focusAdjustment
            Layout.rightMargin: Constants.focusAdjustment
            Layout.preferredWidth: parent.width + 2 * Constants.focusBorderWidth
            title: OnboardingStrings.deviceName
            hint.icon: ""
            sizes: LargeSizes {}
            textField {
                text: deviceName.name
                maximumLength: 32 // It is non-technical length limit for device name => UX choice
            }
        }
    }
}
