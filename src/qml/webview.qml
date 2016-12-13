
import QtQuick 2.2
import QtWebView 1.1
import QtQuick.Controls 2.0

Frame {
    
    property url currenturl: ""
    property int currentwidth: 300
    property int currentheight: 600
    
    visible: true
    
    id: root
    width: currentwidth
    height: currentheight
    
    anchors.margins:0
    
    signal sizeChanged(variant result)
    
    WebView {
        id: webView
        url: root.currenturl
        anchors.fill: parent
        anchors.margins:0
        objectName: "webView"
        onLoadingChanged: runJavaScript("try {\
        document.getElementById('viewerContainer').style.overflow='visible';\
        document.body.style.overflow='hidden';\
        document.getElementById('toolbarViewerMiddle').style.display='none';\
        new Array(document.getElementById('viewer').firstChild.firstChild.offsetWidth, document.getElementById('viewer').offsetHeight, document.getElementById('scaleSelect').options.selectedIndex);\
        } catch (err){\
        document.body.style.overflow='hidden';\
        document.body.firstChild.style.width='100%';\
        document.body.firstChild.style.height='auto';\
        new Array(document.body.firstChild.offsetWidth, document.body.firstChild.offsetHeight);\
        }", 
            function(result){ root.sizeChanged(result); });
    }
    
    Timer {
        interval: 200; running: true; repeat: true
        onTriggered: webView.onLoadingChanged(webView.loadProgress);
    }
}
