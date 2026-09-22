#pragma once

#include <QObject>

// MOC sees ordinary Qt types; only the implementation imports the module.
class Bridge : public QObject {
    Q_OBJECT
    Q_PROPERTY(int value READ value NOTIFY valueChanged)
public:
    explicit Bridge(QObject* parent = nullptr) : QObject(parent) {}
    int value() const { return m_value; }
    Q_INVOKABLE void advance();
signals:
    void valueChanged();
private:
    int m_value = 0;
};
