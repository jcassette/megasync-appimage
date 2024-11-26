pragma Singleton
import QtQuick 2.15

QtObject {

    readonly property string none: ""

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //   Paths
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    readonly property url imagesPath: Qt.resolvedUrl("../../images/")
    readonly property url imagesQmlPath: Qt.resolvedUrl(imagesPath + "qml/")
    readonly property url imagesOnboardingPath: Qt.resolvedUrl(imagesQmlPath + "onboarding/")
    readonly property url imagesSyncsPath: Qt.resolvedUrl(imagesQmlPath + "syncs/")
    readonly property url imagesGuestPath: Qt.resolvedUrl(imagesQmlPath + "guest/")
    readonly property url imagesExclusionsPath: Qt.resolvedUrl(imagesQmlPath + "/sync_exclusions/")
    readonly property url imagesDeviceCentrePath: Qt.resolvedUrl(imagesQmlPath + "/device_center/")

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //   Image paths
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    readonly property url alertCircle: imagesQmlPath + "alert_circle.svg"
    readonly property url alertCircleFilled: imagesQmlPath + "alert_circle_filled.svg"
    readonly property url alertTriangle: imagesQmlPath + "alert_triangle.svg"
    readonly property url arrowRight: imagesQmlPath + "arrow_right.svg"
    readonly property url check: imagesQmlPath + "check.svg"
    readonly property url helpCircle: imagesQmlPath + "help_circle.svg"
    readonly property url indeterminate: imagesQmlPath + "indeterminate.svg"
    readonly property url lock: imagesQmlPath + "lock.svg"
    readonly property url loader: imagesQmlPath + "loader.svg"
    readonly property url smallCircle: imagesQmlPath + "small_circle.svg"
    readonly property url tip: imagesQmlPath + "tip.svg"
    readonly property url megaOutline: imagesQmlPath + "mega_outline.svg"
    readonly property url mega: imagesQmlPath + "mega.svg"
    readonly property url trash: imagesQmlPath + "trash.svg"
    readonly property url xCircle: imagesQmlPath + "x_circle.svg"
    readonly property url smallCheck: imagesQmlPath + "small_check.svg"
    readonly property url checkCircle: imagesQmlPath + "check_circle.svg"
    readonly property url checkCircleFilled: imagesQmlPath + "check_circle_filled.svg"
    readonly property url warning: imagesQmlPath + "warning.png"
    readonly property url twofa: imagesQmlPath + "lock.png"
    readonly property url eye: imagesQmlPath + "eye.svg"
    readonly property url eyeOff: imagesQmlPath + "eye-off.svg"
    readonly property url rocket: imagesQmlPath + "rocket.png"
    readonly property url megaCloud: imagesQmlPath + "mega-cloud.png"
    readonly property url contols: imagesQmlPath + "controls.png"
    readonly property url ok: imagesQmlPath + "ok.png"
    readonly property url threeDots: imagesQmlPath + "three_dots.svg"

    readonly property url building: imagesOnboardingPath + "building.svg"
    readonly property url database: imagesOnboardingPath + "database.svg"
    readonly property url edit: imagesOnboardingPath + "edit.svg"
    readonly property url folder: imagesOnboardingPath + "folder.svg"
    readonly property url fullSync: imagesOnboardingPath + "full_sync.svg"
    readonly property url infinity: imagesOnboardingPath + "infinity.svg"
    readonly property url installationTypeBackups: imagesOnboardingPath + "installation_type_backups.svg"
    readonly property url key: imagesOnboardingPath + "key.svg"
    readonly property url login: imagesOnboardingPath + "login.png"
    readonly property url pc: imagesOnboardingPath + "pc.svg"
    readonly property url pcMega: imagesOnboardingPath + "pc_mega.svg"
    readonly property url person: imagesOnboardingPath + "person.svg"
    readonly property url plus: imagesOnboardingPath + "plus.svg"
    readonly property url resume: imagesOnboardingPath + "resume.svg"
    readonly property url mail: imagesOnboardingPath + "mail.svg"
    readonly property url selectiveSync: imagesOnboardingPath + "selective_sync.svg"
    readonly property url shield_account_free: imagesOnboardingPath + "shield_account_free.svg"
    readonly property url shield_account_lite: imagesOnboardingPath + "shield_account_lite.svg"
    readonly property url shield_account_proI: imagesOnboardingPath + "shield_account_proI.svg"
    readonly property url shield_account_proII: imagesOnboardingPath + "shield_account_proII.svg"
    readonly property url shield_account_proIII: imagesOnboardingPath + "shield_account_proIII.svg"
    readonly property url sync: imagesOnboardingPath + "sync.svg"
    readonly property url syncIcon: imagesOnboardingPath + "syncb.svg"

    readonly property url exit: imagesGuestPath + "exit.svg"
    readonly property url guest: imagesGuestPath + "guest.png"
    readonly property url menu: imagesGuestPath + "menu.svg"
    readonly property url settings: imagesGuestPath + "settings.svg"
    readonly property url warningGuest: imagesGuestPath + "warning.png"
    readonly property url settingUp: imagesGuestPath + "setting_up.png"

    readonly property url refresh: imagesExclusionsPath + "refresh.svg"
    readonly property url xSquare: imagesExclusionsPath + "x-square.svg"
    readonly property url editRule: imagesExclusionsPath + "edit.svg"
    readonly property url chevronDown: imagesExclusionsPath + "chevron-down.svg"
    readonly property url info: imagesExclusionsPath + "info.svg"

    readonly property url syncTypeSelective: imagesSyncsPath + "selective_sync.svg"
    readonly property url syncTypeFull: imagesSyncsPath + "full_sync.svg"

    readonly property url addBackup: imagesDeviceCentrePath + "addbackup.svg"
    readonly property url addSync: imagesDeviceCentrePath + "addsync.svg"
    readonly property url syncFolder: imagesDeviceCentrePath + "sync_folder.svg"
    readonly property url backupFolder: imagesDeviceCentrePath + "backup_folder.svg"

    readonly property url pcWindows: imagesDeviceCentrePath + "pc-windows.svg"
    readonly property url pcMac: imagesDeviceCentrePath + "pc-mac.svg"
    readonly property url pcLinux: imagesDeviceCentrePath + "pc-linux.svg"
    readonly property url statusPaused: imagesDeviceCentrePath + "status-paused.svg"
    readonly property url statusStopped: imagesDeviceCentrePath + "status-stopped.svg"
    readonly property url statusUpdating: imagesDeviceCentrePath + "status-updating.svg"
    readonly property url statusUpToDate: imagesDeviceCentrePath + "status-uptodate.svg"
    readonly property url devices: imagesDeviceCentrePath + "devices.svg"

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //   Standard Icons
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    readonly property url standard_DirIcon: "image://standardicons/SP_DirIcon"

}
