import QtQuick 2.15

import common 1.0

import components.texts 1.0 as Texts

import LoginController 1.0

Item {
    id: root
    
    readonly property int contentSpacing: 24

    function setInitialFocusPosition() {
        window.requestPageFocus();
    }

    Texts.Text {
        id: statusText

        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: root.bottom
            bottomMargin: (Constants.defaultWindowMargin - statusText.height) / 2
        }
        font.pixelSize: Texts.Text.Size.SMALL
        color: ColorTheme.textSecondary
        text: {
            switch(loginControllerAccess.state) {
                case LoginController.FETCHING_NODES:
                case LoginController.FETCHING_NODES_2FA:
                    return OnboardingStrings.statusFetchNodes;
                case LoginController.LOGGING_IN:
                    return OnboardingStrings.statusLogin;
                case LoginController.LOGGING_IN_2FA_VALIDATING:
                    return OnboardingStrings.status2FA;
                case LoginController.CREATING_ACCOUNT:
                    return OnboardingStrings.statusSignUp;
                default:
                    return "";
            }
        }
    }

}
