
import QtQuick 2.2
import QtWebView 1.0
import QtQuick.Controls 2.0

Frame {
    visible: true
    
    id: root
    width: 800
    height: 600
    
    property url currenturl: "http://www.google.com"

    WebView {
        id: webView
        anchors.fill: parent
        url: root.currenturl
        objectName: "webView"
    }
}
