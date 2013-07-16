#include <QDebug>
#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlProperty>
#include <QQuickView>
#include <QQuickItem>
#include <QQmlEngine>
#include <QQmlContext>

class KlaatuApplication : public QGuiApplication {
    Q_OBJECT
public:
    KlaatuApplication(int argc, char **argv) : QGuiApplication(argc, argv) {}
public slots:
    void showMouse(bool);
};

