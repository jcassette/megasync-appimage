import QtQuick 2.15
import QtQuick.Layouts 1.15

import common 1.0

import components.texts 1.0 as Texts
import components.textFields 1.0
import components.buttons 1.0

import QmlClipboard 1.0

FocusScope {
    id: root

    property string key: digit1.text + digit2.text + digit3.text
                            + digit4.text + digit5.text + digit6.text
    property bool hasError: false

    signal allDigitsFilled

    function pastePin() {
        const regex = RegexExpressions.allDigits2FA;
        var pin = QmlClipboard.text().slice(0, 6);
        if (!regex.test(pin)) {
            console.warn("Invalid 2FA pin format pasted");
            return;
        }

        digit1.text = pin.charAt(0);
        digit2.text = pin.charAt(1);
        digit3.text = pin.charAt(2);
        digit4.text = pin.charAt(3);
        digit5.text = pin.charAt(4);
        digit6.text = pin.charAt(5);
    }

    Layout.leftMargin: Constants.focusAdjustment

    onKeyChanged: {
        if(key.length === 6) {
            allDigitsFilled();
        }
        else if (key.length === 0) {
            digit1.forceActiveFocus();
        }
    }

    ColumnLayout {
        id: columnLayout

        spacing: 20

        RowLayout {
            id: mainLayout

            Layout.preferredHeight: digit1.height
            spacing: 0

            TwoFADigit {
                id: digit1

                focus: true
                error: hasError
                next: digit2
                onPastePressed: {
                    pastePin();
                }
            }

            TwoFADigit {
                id: digit2

                error: hasError
                next: digit3
                previous: digit1
                onPastePressed: {
                    pastePin();
                }
            }

            TwoFADigit {
                id: digit3

                error: hasError
                next: digit4
                previous: digit2
                onPastePressed: {
                    pastePin();
                }
            }

            TwoFADigit {
                id: digit4

                error: hasError
                next: digit5
                previous: digit3
                onPastePressed: {
                    pastePin();
                }
            }

            TwoFADigit {
                id: digit5

                error: hasError
                next: digit6
                previous: digit4
                onPastePressed: {
                    pastePin();
                }
            }

            TwoFADigit {
                id: digit6

                error: hasError
                previous: digit5
                onPastePressed: {
                    pastePin();
                }
            }

        } // RowLayout: mainLayout

        Texts.NotificationText {
            id: notification

            Layout.leftMargin: Constants.focusAdjustment
            Layout.preferredWidth: root.width + Constants.focusAdjustment
            Layout.preferredHeight: notification.height
            title: qsTranslate("OnboardingStrings", "Incorrect 2FA code")
            text: Strings.tryAgain
            type: Constants.MessageType.ERROR
            icon: Images.lock
            time: 2000
            visible: hasError

            onVisibilityTimerFinished: {
                hasError = false;
                digit1.textField.text = "";
                digit2.textField.text = "";
                digit3.textField.text = "";
                digit4.textField.text = "";
                digit5.textField.text = "";
                digit6.textField.text = "";
                root.forceActiveFocus();
            }
        }

        LinkButton {
            id: helpButtonItem

            Layout.leftMargin: -sizes.horizontalPadding
            text: qsTranslate("OnboardingStrings", "Problem with two-factor authentication?")
            url: Links.recovery
            icons {
                source: Images.helpCircle
                position: Icon.Position.LEFT
            }
            visible: !root.hasError
            sizes: SmallSizes { borderLess: true }
        }

        Shortcut {
            id: shortcutItem

            sequence: [ StandardKey.Paste ]
            onActivated: {
                pastePin();
            }
        }

    } // ColumnLayout: columnLayout

}


