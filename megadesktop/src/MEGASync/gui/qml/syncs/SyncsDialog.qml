import QtQuick 2.0

import common 1.0

import components.views 1.0
import components.steps 1.0

import SyncsQmlDialog 1.0
import SyncInfo 1.0

SyncsQmlDialog {
    id: window

    readonly property int syncOrigin: SyncInfo.MAIN_APP_ORIGIN

    title: SyncsStrings.syncsWindowTitle
    visible: true
    modality: Qt.NonModal
    width: 640
    height: 620
    maximumHeight: height
    maximumWidth: width
    minimumHeight: height
    minimumWidth: width

    Column {
        id: contentItem

        anchors.fill: parent

        StepPanel {
            id: stepPanelItem

            width: parent.width
            step1String: SyncsStrings.syncTitle
            step2String: SyncsStrings.sync
            helpUrl: Links.setUpSyncs
        }

        Rectangle {
            id: syncsContentItem

            width: parent.width
            height: parent.height - stepPanelItem.height
            color: ColorTheme.surface1

            readonly property string syncsFlow: "syncsFlow"
            readonly property string resume: "resume"

            state: syncsFlow
            states: [
                State {
                    name: syncsContentItem.syncsFlow
                    StateChangeScript {
                        script: stackView.replace(syncsFlowPage);
                    }
                },
                State {
                    name: syncsContentItem.resume
                    StateChangeScript {
                        script: stackView.replace(resumePage);
                    }
                    PropertyChanges { target: stepPanelItem; state: stepPanelItem.stepCurrentDone; }
                }
            ]

            StackViewBase {
                id: stackView

                anchors {
                    fill: parent
                    margins: Constants.defaultWindowMargin
                }

                Component {
                    id: syncsFlowPage

                    SyncsPage {
                        id: syncsFlowItem

                        stepPanelRef : stepPanelItem
                        syncsContentItemRef: syncsContentItem
                    }
                }

                Component {
                    id: resumePage

                    ResumeSyncsPage {
                        id: resumeSyncsPageItem

                        footerButtons.leftPrimary.visible: false
                    }
                }
            }

        } // Rectangle: syncsContentItem
    }

}
