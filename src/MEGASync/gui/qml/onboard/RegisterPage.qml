import QtQuick 2.15

import LoginController 1.0

RegisterPageForm {
    id: root

    nextButton.onClicked: {
        if(registerContent.error()) {
            return;
        }

        loginControllerAccess.createAccount(registerContent.email.text.trim(),
                                            registerContent.password.text,
                                            registerContent.firstName.text,
                                            registerContent.lastName.text);
        window.creatingAccount = true;
    }

    loginButton.onClicked: {
        loginControllerAccess.state = LoginController.LOGGED_OUT;
    }

    Connections {
        target: window

        function onInitializePageFocus() {
            registerContent.firstName.forceActiveFocus();
        }
    }

}
