#include "bridge.h"

#include <QCoreApplication>
#include <QDebug>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QVariant>
#include <memory>

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    qmlRegisterType<Bridge>("Runmark.Spike", 1, 0, "Bridge");
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(R"(
        import QtQml
        import Runmark.Spike 1.0
        QtObject {
            property Bridge model: Bridge {}
            property int observed: model.value
            function advance() { model.advance() }
        }
    )", QUrl("qrc:/spike.qml"));
    std::unique_ptr<QObject> object(component.create());
    if (!object) {
        qCritical() << component.errors();
        return 1;
    }
    if (object->property("observed").toInt() != 0
        || !QMetaObject::invokeMethod(object.get(), "advance")
        || object->property("observed").toInt() != 1) {
        qCritical() << "Module -> QObject -> QML binding failed";
        return 1;
    }
    qInfo() << "Module -> QObject -> QML binding passed";
    return 0;
}
