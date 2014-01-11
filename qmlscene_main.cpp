/*
  Klaatu_qmlscene - A shell for testing Klaatu programs
 */

#define LOG_TAG "Klaatu"
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>

#include <QDebug>
#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlProperty>
#include <QQuickView>
#include <QQuickItem>
#include <QQmlEngine>
#include <QQmlContext>

#if defined(HAVE_PTHREADS)
#include <pthread.h>
#include <sys/resource.h>
#endif

#include "screencontrol.h"
#include "audiocontrol.h"
#include "battery.h"
#include "inputcontext.h"
#include "settings.h"
#include "wifi.h"
#include "klaatuapplication.h"
#include "sensors.h"
#include "screenorientation.h"
#include "power.h"

#include <QtGui/private/qinputmethod_p.h>
#include <qpa/qplatforminputcontext.h>
#include <qpa/qplatformintegration.h>
#include <QList>
#include <QPersistentModelIndex>
#include "event_thread.h"

using namespace android;

QString progname;

static void usage(int code=0)
{
    qWarning("Usage: %s [ARGS] FILENAME\n"
	     "\n"
	     "Valid args:\n"
	     "   -i|--import DIRNAME     Add to QML import path\n"
	     "   -d|--device DEVICE      Set up input methods for hardware\n"
	     "\n"
	     "The DEVICE value may be 'nexus'\n"
	     "The FILENAME should be a QML file to load\n", qPrintable(progname));
    exit(code);
}

Q_DECLARE_METATYPE(QList<QPersistentModelIndex>)

static void registerQmlTypes()
{
    qmlRegisterType<AudioFile>("Klaatu", 1, 0, "AudioFile");

    qmlRegisterUncreatableType<AudioControl>("Klaatu", 1, 0, "AudioControl","Single instance");
    qmlRegisterUncreatableType<ScreenControl>("Klaatu", 1, 0, "ScreenControl","Single instance");
    qmlRegisterUncreatableType<Battery>("Klaatu", 1, 0, "Battery","Single instance");
    qmlRegisterUncreatableType<InputContext>("Klaatu", 1, 0, "InputContext","Single instance");
    qmlRegisterUncreatableType<Settings>("Klaatu", 1, 0, "Settings","Single instance");
    qmlRegisterUncreatableType<Wifi>("Klaatu", 1, 0, "Wifi","Single instance");
    qmlRegisterUncreatableType<Wifi>("Klaatu", 1, 0, "Sensors","Single instance");
    qmlRegisterUncreatableType<Wifi>("Klaatu", 1, 0, "ScreenOrientation","Single instance");
    qmlRegisterUncreatableType<Wifi>("Klaatu", 1, 0, "Power","Single instance");

    qRegisterMetaType<QSet<int> >();
    qRegisterMetaType<QList<QPersistentModelIndex> >();
}


int main(int argc, char **argv)
{
#if defined(HAVE_PTHREADS)
    setpriority(PRIO_PROCESS, 0, ANDROID_PRIORITY_DISPLAY);
#endif
    KlaatuApplication app(argc, argv);
    app.setApplicationName("Klaatu_QMLScene");
    app.setOrganizationName("Klaatu");
    app.setOrganizationDomain("klaatu.com");

    QString     device;
    QStringList imports;
    QStringList args = QGuiApplication::arguments();
    progname = args.takeFirst();

    while (args.size()) {
	QString arg = args.at(0);
	if (!arg.startsWith('-'))
	    break;
	args.removeFirst();
	if (arg == QStringLiteral("-help"))
	    usage();
	else if (arg == QStringLiteral("--import") || arg == QStringLiteral("-i")) {
	    if (!args.size())
		usage();
	    imports.append(args.takeFirst());
	}
	else if (arg == QStringLiteral("--device") || arg == QStringLiteral("-d")) {
	    if (!args.size())
		usage();
	    device = args.takeFirst();
	}
	else {
	    qWarning("Unexpected argument '%s'", qPrintable(arg));
	    usage(1);
	}
    }

    if (args.size() != 1)
	usage(1);

    qDebug() << "Checking for input device" << device;

    registerQmlTypes();

    QQuickView *view = new QQuickView;
    view->setResizeMode(QQuickView::SizeRootObjectToView);

    QQmlEngine *engine = view->engine();
    for (int i = 0 ; i < imports.size() ; i++)
	engine->addImportPath(imports.at(i));

    ProcessState::self()->startThreadPool();
    ScreenControl *screen = ScreenControl::instance();
    engine->rootContext()->setContextProperty(QStringLiteral("screencontrol"), 
					      screen);
    engine->rootContext()->setContextProperty(QStringLiteral("audiocontrol"), 
					      AudioControl::instance());
    engine->rootContext()->setContextProperty(QStringLiteral("battery"), 
					      Battery::instance());
    engine->rootContext()->setContextProperty(QStringLiteral("settings"),
					      Settings::instance());
    engine->rootContext()->setContextProperty(QStringLiteral("sensors"),
                                              Sensors::instance()),
    engine->rootContext()->setContextProperty(QStringLiteral("screenorientation"),
                                              ScreenOrientation::instance()),
    engine->rootContext()->setContextProperty(QStringLiteral("power"),
                                              Power::instance()),
#ifndef KLAATU_NO_WIFI
    engine->rootContext()->setContextProperty(QStringLiteral("wifi"),
					      Wifi::instance());
#endif
    InputContext *context = InputContext::instance();
    QInputMethodPrivate *inputMethodPrivate = QInputMethodPrivate::get(qApp->inputMethod());
    inputMethodPrivate->testContext = context;
    engine->rootContext()->setContextProperty(QStringLiteral("inputcontext"), context);
    QObject::connect(&app, SIGNAL(focusObjectChanged(QObject *)), context, SLOT(focusObjectChanged(QObject *)));

    view->setSource(args.at(0));

    // We have to create the EventThread after setting the view source
    // so that the OpenGL ES context is initialized before we try to
    // set the mouse cursor on or off.
    android::sp<EventThread> ethread;
    ethread = new EventThread(screen, new android::EventHub());
    ethread->run("InputReader", PRIORITY_URGENT_DISPLAY);

    QObject::connect(engine, SIGNAL(quit()), &app, SLOT(quit()));
    view->showFullScreen();

    return app.exec();
}

