import QtQuick 2.15

import common 1.0
import onboard 1.0
import QmlDeviceName 1.0

DeviceNamePageForm {
    id: root

    signal deviceNameMoveToSyncType

    footerButtons.leftPrimary {
        text: Strings.skip
        onClicked: {
            window.close();
        }
    }

    footerButtons.rightPrimary.onClicked: {
        var emptyText = deviceNameTextField.text.length === 0;
        if(emptyText) {
            deviceNameTextField.hint.textColor = ColorTheme.textError;
        }
        deviceNameTextField.error = emptyText;
        deviceNameTextField.hint.text = emptyText ? OnboardingStrings.errorEmptyDeviceName : "";
        deviceNameTextField.hint.visible = emptyText;

        if(emptyText) {
            return;
        }

        if(!deviceName.setDeviceName(deviceNameTextField.text)) {
            root.deviceNameMoveToSyncType();
        }
    }

    deviceNameTextField.onTextChanged: {
        deviceNameTextField.error = false;
        deviceNameTextField.hint.text = "";
        deviceNameTextField.hint.visible = false;

        if(deviceNameTextField.text.length >= deviceNameTextField.textField.maximumLength) {
            deviceNameTextField.hint.textColor = ColorTheme.textSecondary;
            deviceNameTextField.hint.text = OnboardingStrings.errorDeviceNameLimit;
            deviceNameTextField.hint.visible = true;
        }
    }

    QmlDeviceName {
        id: deviceName

        onDeviceNameSet: {
            root.deviceNameMoveToSyncType();
        }
    }

    Connections {
        target: window

        function onInitializePageFocus() {
            deviceNameTextField.forceActiveFocus();
        }
    }
}

