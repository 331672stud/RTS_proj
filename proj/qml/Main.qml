import QtQuick
import QtPositioning
import QtLocation

Window {
    width: 640
    height: 480
    visible: true
    title: qsTr("Hello World")

    ListModel {
        id: edgeModel
    }

    ListModel {
        id: pointModel
    }

    Map {
        id: map

        anchors.fill: parent

        focus: true
        activeFocusOnTab: true

        Keys.onPressed: function(event) {

            const step = 0.002 * Math.pow(2, 15 - map.zoomLevel)
            const zoomStep = 0.5

            if (event.key === Qt.Key_Up) {
                map.center = QtPositioning.coordinate(
                    map.center.latitude + step,
                    map.center.longitude
                )
            }

            if (event.key === Qt.Key_Down) {
                map.center = QtPositioning.coordinate(
                    map.center.latitude - step,
                    map.center.longitude
                )
            }

            if (event.key === Qt.Key_Left) {
                map.center = QtPositioning.coordinate(
                    map.center.latitude,
                    map.center.longitude - step
                )
            }

            if (event.key === Qt.Key_Right) {
                map.center = QtPositioning.coordinate(
                    map.center.latitude,
                    map.center.longitude + step
                )
            }

            if (event.key === Qt.Key_Plus || event.key === Qt.Key_Equal) {
                map.zoomLevel += zoomStep
            }

            if (event.key === Qt.Key_Minus) {
                map.zoomLevel -= zoomStep
            }

            event.accepted = true
        }

        plugin: Plugin {
            name: "osm"

            PluginParameter {
                name: "osm.mapping.custom.host"
                value: "https://tile.openstreetmap.org/"
            }
        }

        activeMapType: map.supportedMapTypes[map.supportedMapTypes.length - 1]

        center: QtPositioning.coordinate(52.23, 21)
        zoomLevel: 11


        MapItemView {
            model: pointModel

            delegate: MapQuickItem {

                coordinate: QtPositioning.coordinate(lat, lon)

                sourceItem: Rectangle {
                    width: 12
                    height: 12
                    radius: 6
                    color: "blue"
                }

                anchorPoint.x: 6
                anchorPoint.y: 6
            }
        }

        MapItemView {
            model: edgeModel

            delegate: MapPolyline {
                line.width: 2
                line.color: color

                path: [
                    QtPositioning.coordinate(lat1, lon1),
                    QtPositioning.coordinate(lat2, lon2)
                ]
            }
        }
    }
    Connections {
        target: backend

        function onGraphEdgeUpdated(u, v, weight,
                                    lat1, lon1,
                                    lat2, lon2)
        {
            edgeModel.append({
                "u": u,
                "v": v,
                "lat1": lat1,
                "lon1": lon1,
                "lat2": lat2,
                "lon2": lon2,
                "color": "red"
            })
        }

        function onWaypointsReceived(points) {

            for (const p of points) {
                pointModel.append({
                    "lat": p.lat,
                    "lon": p.lon
                })
            }
        }
    }
}





