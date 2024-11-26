import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtGraphicalEffects 1.15

import common 1.0

import components.texts 1.0 as Texts
import components.images 1.0

import AccountInfoData 1.0

Item {
    id: root

    readonly property string accountTypeFree: qsTr("Free")
    readonly property string accountTypeProI: qsTranslate("Utilities", "Pro I")
    readonly property string accountTypeProII: qsTranslate("Utilities", "Pro II")
    readonly property string accountTypeProIII: qsTranslate("Utilities", "Pro III")
    readonly property string accountTypeLite: qsTranslate("Utilities", "Pro Lite")
    readonly property string accountTypeStarter: qsTranslate("Utilities", "MEGA Starter")
    readonly property string accountTypeBasic: qsTranslate("Utilities", "MEGA Basic")
    readonly property string accountTypeEssential: qsTranslate("Utilities", "MEGA Essential")
    readonly property string accountTypeBusiness: qsTr("Business")
    readonly property string accountTypeProFlexi: qsTr("Pro Flexi")
    readonly property string availableStorage: qsTr("Available storage:")
    readonly property string storageSpace: qsTr("Storage space:")

    function getAccountTypeImage() {
        switch(AccountInfoData.type) {
            case AccountInfoData.ACCOUNT_TYPE_FREE:
                return Images.shield_account_free;
            case AccountInfoData.ACCOUNT_TYPE_PROI:
                return Images.shield_account_proI;
            case AccountInfoData.ACCOUNT_TYPE_PROII:
                return Images.shield_account_proII;
            case AccountInfoData.ACCOUNT_TYPE_PROIII:
                return Images.shield_account_proIII;
            case AccountInfoData.ACCOUNT_TYPE_LITE:
                return Images.shield_account_lite;
            case AccountInfoData.ACCOUNT_TYPE_BUSINESS:
                return Images.building;
            case AccountInfoData.ACCOUNT_TYPE_PRO_FLEXI:
                return Images.infinity;
            default:
                // No image displayed for starter, basic and essential accounts
                return "";
        }
    }

    function getAccountTypeText() {
        switch(AccountInfoData.type) {
            case AccountInfoData.ACCOUNT_TYPE_FREE:
                return accountTypeFree;
            case AccountInfoData.ACCOUNT_TYPE_PROI:
                return accountTypeProI;
            case AccountInfoData.ACCOUNT_TYPE_PROII:
                return accountTypeProII;
            case AccountInfoData.ACCOUNT_TYPE_PROIII:
                return accountTypeProIII;
            case AccountInfoData.ACCOUNT_TYPE_LITE:
                return accountTypeLite;
            case AccountInfoData.ACCOUNT_TYPE_STARTER:
                return accountTypeStarter;
            case AccountInfoData.ACCOUNT_TYPE_BASIC:
                return accountTypeBasic;
            case AccountInfoData.ACCOUNT_TYPE_ESSENTIAL:
                return accountTypeEssential;
            case AccountInfoData.ACCOUNT_TYPE_BUSINESS:
                return accountTypeBusiness;
            case AccountInfoData.ACCOUNT_TYPE_PRO_FLEXI:
                return accountTypeProFlexi;
            default:
                return "";
        }
    }

    width: parent.width
    height: 48

    Component.onCompleted: {
        AccountInfoData.requestAccountInfoData();
    }

    Rectangle {
        id: background

        anchors.fill: parent
        color: ColorTheme.pageBackground
        radius: 8
        border {
            color: ColorTheme.borderDisabled
            width: 1
        }

        RowLayout {
            id: internalLayout

            anchors.fill: parent
            spacing: 0
            visible: AccountInfoData.type !== AccountInfoData.ACCOUNT_TYPE_NOT_SET

            RowLayout {
                id: leftLayout

                Layout.alignment: Qt.AlignLeft
                Layout.leftMargin: 20
                spacing: 7

                SvgImage {
                    id: typeImage

                    source: getAccountTypeImage()
                    height: 16
                    width: 16
                    sourceSize: Qt.size(width, height)
                    opacity: enabled ? 1.0 : 0.2
                    visible: AccountInfoData.type !== AccountInfoData.ACCOUNT_TYPE_STARTER
                             && AccountInfoData.type !== AccountInfoData.ACCOUNT_TYPE_BASIC
                             && AccountInfoData.type !== AccountInfoData.ACCOUNT_TYPE_ESSENTIAL
                }

                Texts.Text {
                    id: typeText

                    Layout.alignment: Qt.AlignLeft
                    font.weight: Font.DemiBold
                    font.pixelSize: Texts.Text.Size.NORMAL
                    text: getAccountTypeText()
                }
            }

            RowLayout {
                id: rightLayout

                Layout.alignment: Qt.AlignRight
                Layout.rightMargin: 18
                visible: AccountInfoData.type !== AccountInfoData.ACCOUNT_TYPE_BUSINESS
                         && AccountInfoData.type !== AccountInfoData.ACCOUNT_TYPE_PRO_FLEXI
                         && AccountInfoData.type !== AccountInfoData.ACCOUNT_TYPE_NOT_SET

                Texts.Text {
                    id: constantText

                    text: AccountInfoData.belowMinUsedStorageThreshold
                          ? availableStorage
                          : storageSpace
                    font.weight: Font.DemiBold
                    font.pixelSize: Texts.Text.Size.SMALL
                }

                Texts.Text {
                    id: usedStorageText

                    font.weight: Font.DemiBold
                    text: AccountInfoData.usedStorage
                    visible: !AccountInfoData.belowMinUsedStorageThreshold
                    font.pixelSize: Texts.Text.Size.SMALL
                }

                Texts.SecondaryText {
                    id: separatorText

                    font.weight: Font.DemiBold
                    text: "/"
                    visible: !AccountInfoData.belowMinUsedStorageThreshold
                    font.pixelSize: Texts.Text.Size.SMALL
                }

                Texts.SecondaryText {
                    id: totalStorage

                    font.weight: Font.DemiBold
                    text: AccountInfoData.totalStorage
                    font.pixelSize: Texts.Text.Size.SMALL
                }
            }

        } // RowLayout: internalLayout

    } // Rectangle: background

    DropShadow {
        id: shadow

        anchors.fill: parent
        horizontalOffset: 0
        verticalOffset: 5
        radius: 5.0
        samples: 11
        cached: true
        color: "#0d000000"
        source: background
        visible: parent.enabled
    }
}
