import QtQuick 2.15
import QtQuick.Controls 2.15

import common 1.0

import components.views 1.0

import BackupsProxyModel 1.0

Item {
    id: root

    required property Component selectFoldersPage
    required property Component confirmFoldersPage

    property alias backupsProxyModel: backupsProxyModel

    readonly property string selectBackup: "selectBackup"
    readonly property string confirmBackup: "confirmBackup"

    // Added to avoid qml warning.
    function setInitialFocusPosition() {}

    signal backupFlowMoveToFinal(bool success)
    signal backupFlowMoveToBack

    state: root.selectBackup
    states: [
        State {
            name: root.selectBackup
            StateChangeScript {
                script: view.replace(selectFoldersPage);
            }
        },
        State {
            name: root.confirmBackup
            StateChangeScript {
                script: view.replace(confirmFoldersPage);
            }
        }
    ]

    StackViewBase {
        id: view

        anchors.fill: parent
        onCurrentItemChanged: {
            currentItem.setInitialFocusPosition();
        }
    }

    BackupsProxyModel {
        id: backupsProxyModel
    }

    Connections {
        id: selectFolderBackupNavigationConnection

        target: view.currentItem
        ignoreUnknownSignals: true

        function onSelectFolderMoveToBack() {
            root.backupFlowMoveToBack();
        }

        function onSelectFolderMoveToConfirm() {
            backupsProxyModel.selectedFilterEnabled = true;
            root.state = root.confirmBackup;
        }
    }

    Connections {
        id: confirmFolderBackupNavigationConnection

        target: view.currentItem
        ignoreUnknownSignals: true

        function onConfirmFoldersMoveToSelect() {
            backupsProxyModel.selectedFilterEnabled = false;
            root.state = root.selectBackup;
        }

        function onConfirmFoldersMoveToFinal(success) {
            root.backupFlowMoveToFinal(success);
        }
    }
}
