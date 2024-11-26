import QtQuick 2.15

import common 1.0

ResumePageForm {
    id: root

    footerButtons {
        rightSecondary.onClicked: {
            backupsAccess.openBackupsTabInPreferences();
            window.accept();
        }

        rightPrimary.onClicked: {
            window.accept();
        }
    }

}
