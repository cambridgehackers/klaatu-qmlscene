/*
 */

#include "inputcontext.h"
#include <QDebug>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QTransform>
#include <QWindow>

InputContext *InputContext::instance()
{
    static InputContext *s_inputcontext;
    if (!s_inputcontext) 
	s_inputcontext = new InputContext;
    return s_inputcontext;
}

InputContext::InputContext()
    : mVisible(false)
    , mAnimating(false)
    , mFocusObject(NULL)
{
}

void InputContext::focusObjectChanged(QObject *object)
{
    // qDebug() << "____focus object is now" << object;
}

InputContext::~InputContext()
{
}

bool InputContext::isValid() const
{
    return true;
}

void InputContext::reset()
{
    // qDebug() << "*****" << Q_FUNC_INFO;
    mFocusObject = NULL;
}

static InputContext::KeyboardType hintToKeyboardType(Qt::InputMethodHints hints)
{
    hints &= Qt::ImhExclusiveInputMask;
    if (hints == Qt::ImhFormattedNumbersOnly)
	return InputContext::TypeNumbers;
    if (hints == Qt::ImhDigitsOnly)
	return InputContext::TypeDigits;
    if (hints == Qt::ImhDialableCharactersOnly)
	return InputContext::TypeDialable;
    if (hints == Qt::ImhEmailCharactersOnly)
	return InputContext::TypeEmail;
    if (hints == Qt::ImhUrlCharactersOnly)
	return InputContext::TypeUrl;
    return InputContext::TypeNormal;
}

void InputContext::processInputMethodQuery(Qt::InputMethodQueries queries)
{
    // qDebug() << "****" << Q_FUNC_INFO;

    if (!QGuiApplication::focusObject())
	return;

    QInputMethodQueryEvent query(queries);
    QGuiApplication::sendEvent(QGuiApplication::focusObject(), &query);

    if (queries & Qt::ImSurroundingText) {
	// qDebug() << "****" << "Surrounding text" << query.value(Qt::ImSurroundingText);
    }
    if (queries & Qt::ImCursorPosition) {
	mCursorPosition = query.value(Qt::ImCursorPosition).toInt();
	// qDebug() << "****" << "Cursor position" << mCursorPosition;
    }
    if (queries & Qt::ImAnchorPosition) {
	mAnchorPosition = query.value(Qt::ImAnchorPosition).toInt();
	// qDebug() << "****" << "Anchor position" << mAnchorPosition;
    }
    if (queries & Qt::ImCursorRectangle) {
       QRect rect = query.value(Qt::ImCursorRectangle).toRect();
       rect = QGuiApplication::inputMethod()->inputItemTransform().mapRect(rect);
       QWindow *window = QGuiApplication::focusWindow();
       if (window) {
	   // qDebug() << "****" << "Cursor rectangle" << QRect(window->mapToGlobal(rect.topLeft()), rect.size());
       }
    }

    if (queries & Qt::ImCurrentSelection) {
        // qDebug() << "****" << "Has selection" << !query.value(Qt::ImCurrentSelection).toString().isEmpty();
    }

    if (queries & Qt::ImHints) {
        Qt::InputMethodHints hints = Qt::InputMethodHints(query.value(Qt::ImHints).toUInt());

	// qDebug() << "****" << "Prediction enabled" << !(hints & Qt::ImhNoPredictiveText);
        // qDebug() << "****" << "Autocapitalization enabled" << !(hints & Qt::ImhNoAutoUppercase);
        // qDebug() << "****" << "Hidden text" << ((hints & Qt::ImhHiddenText) != 0);
	KeyboardType type = hintToKeyboardType(hints);
	// qDebug() << "****" << "Keyboard type" << type;
	if (type != mKeyboardType) {
	    mKeyboardType = type;
	    emit keyboardTypeChanged();
	}
    }
}

void InputContext::update(Qt::InputMethodQueries queries)
{
    // qDebug() << "****" << Q_FUNC_INFO;
    processInputMethodQuery(queries);
}

void InputContext::showInputPanel()
{
    // qDebug() << "*****" << Q_FUNC_INFO;
    processInputMethodQuery(Qt::ImCursorPosition | Qt::ImAnchorPosition | Qt::ImHints);
    emit show();
}

void InputContext::hideInputPanel()
{
    // qDebug() << "*****" << Q_FUNC_INFO;
    emit hide();
}

void InputContext::setFocusObject(QObject *object)
{
    // qDebug() << "*****" << Q_FUNC_INFO;
    mFocusObject = object;
}


void InputContext::setVisible(bool value)
{
    if (mVisible != value) {
	mVisible = value;
	emitInputPanelVisibleChanged();
	emit visibleChanged();
    }
}

void InputContext::setAnimating(bool value)
{
    if (mAnimating != value) {
	mAnimating = value;
	emitAnimatingChanged();
	emit animatingChanged();
    }
}

const char *keyboard_types[] = { "normal", "numbers", "digits", "dialable", "email", "url" };
QString InputContext::keyboardTypeString() const
{
    return keyboard_types[mKeyboardType];
}


/**
   Normal insertion of text into the focus object.   This effectively
   combines "preedit" and "commit" into a single step
 */

void InputContext::insertKey(const QString& text)
{
    QInputMethodEvent event;
    event.setCommitString(text);
    QCoreApplication::sendEvent(QGuiApplication::focusObject(), &event);
}

void InputContext::deleteKey()
{
    // qDebug() << Q_FUNC_INFO;
    if (mCursorPosition > 0) {
	QInputMethodEvent event;
	event.setCommitString(QString(), -1, 1);
	QCoreApplication::sendEvent(QGuiApplication::focusObject(), &event);
    }
}

void InputContext::setKeyboardRect(const QRectF& rect)
{
    // qDebug() << Q_FUNC_INFO << rect;
    if (rect != mKeyboardRect) {
	mKeyboardRect = rect;
	emitKeyboardRectChanged();
    }
}

