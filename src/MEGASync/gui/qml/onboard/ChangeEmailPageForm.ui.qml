import QtQuick 2.15
import QtQuick.Layouts 1.15

import common 1.0

import components.buttons 1.0
import components.texts 1.0 as Texts
import components.textFields 1.0

StackViewPage {
    id: root

    property alias emailTextField: emailTextField
    property alias cancelButton: cancelButton
    property alias resendButton: resendButton

    ColumnLayout {
        id: layout

        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }
        spacing: contentSpacing

        Texts.Text {
            id: title

            Layout.fillWidth: true
            text: OnboardingStrings.changeEmailTitle
            font.pixelSize: Texts.Text.Size.LARGE
        }

        Texts.Text {
            id: bodyText

            Layout.preferredWidth: layout.width
            text: OnboardingStrings.changeEmailBodyText
            font.pixelSize: Texts.Text.Size.MEDIUM
        }

        EmailTextField {
            id: emailTextField

            title: OnboardingStrings.email
            Layout.preferredWidth: layout.width + 2 * Constants.focusBorderWidth
            Layout.leftMargin: Constants.focusAdjustment
        }
    }

    RowLayout {
        id: buttonsLayout

        anchors {
            right: root.right
            bottom: root.bottom
            rightMargin: Constants.focusAdjustment
            bottomMargin: Constants.defaultWindowMargin + Constants.focusAdjustment
        }
        spacing: 0

        OutlineButton {
            id: cancelButton

            text: Strings.cancel
        }

        PrimaryButton {
            id: resendButton

            text: OnboardingStrings.resend
            icons.source: Images.mail
        }
    }
}
