import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15

import common 1.0

import components.texts 1.0 as Texts
import components.pages 1.0

import onboard 1.0
import syncs 1.0

import LoginController 1.0

FooterButtonsPage {
    id: root

    property alias buttonGroup: buttonGroupComp
    property alias syncButton: syncButtonItem

    footerButtons.rightPrimary.enabled: false

    ColumnLayout {
        id: mainLayout

        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }
        spacing: Constants.defaultComponentSpacing

        HeaderTexts {
            id: headerItem

            title: loginControllerAccess.newAccount
                   ? OnboardingStrings.welcomeToMEGA
                   : OnboardingStrings.letsGetYouSetUp
            description: OnboardingStrings.chooseInstallation
            spacing: 36
            descriptionWeight: Font.DemiBold
            descriptionColor: ColorTheme.textPrimary
        }

        ButtonGroup {
            id: buttonGroupComp
        }

        ColumnLayout {
            id: buttonsLayout

            spacing: 12

            SyncsHorizontalButton {
                id: syncButtonItem

                Layout.leftMargin: -syncButtonItem.focusBorderWidth
                Layout.rightMargin: -syncButtonItem.focusBorderWidth
                title: SyncsStrings.sync
                description: OnboardingStrings.syncButtonDescription
                imageSource: Images.sync
                type: Constants.SyncType.SYNC
                ButtonGroup.group: buttonGroupComp
            }

            SyncsHorizontalButton {
                id: backupsButton

                Layout.leftMargin: -backupsButton.focusBorderWidth
                Layout.rightMargin: -backupsButton.focusBorderWidth
                title: OnboardingStrings.backup
                description: OnboardingStrings.backupButtonDescription
                imageSource: Images.installationTypeBackups
                type: Constants.SyncType.BACKUP
                ButtonGroup.group: buttonGroupComp
            }
        }
    }

}
