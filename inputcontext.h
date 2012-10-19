#ifndef INPUTCONTEXT_H
#define INPUTCONTEXT_H

#include <qpa/qplatforminputcontext.h>
#include <QQuickItem>

QT_BEGIN_NAMESPACE

class InputContext : public QPlatformInputContext
{
    Q_OBJECT
    Q_ENUMS(KeyboardType)
    Q_PROPERTY(bool visible READ isInputPanelVisible WRITE setVisible NOTIFY visibleChanged)
    Q_PROPERTY(bool isAnimating READ isAnimating WRITE setAnimating NOTIFY animatingChanged)
    Q_PROPERTY(KeyboardType keyboardType READ keyboardType NOTIFY keyboardTypeChanged)

public:
    enum KeyboardType { TypeNormal, TypeNumbers, TypeDigits, TypeDialable,
			TypeEmail, TypeUrl };
    static InputContext *instance();
    ~InputContext();

    // -- QPlatformInputContext --
    bool isValid() const;
    void reset();
//    void commit();
    void update(Qt::InputMethodQueries);
//    void invokeAction(QInputMethod::Action, int cursorPosition);
//    bool filterEvent(const QEvent *event);
    QRectF keyboardRect() const { return mKeyboardRect; }

    bool isAnimating() const { return mAnimating; }
    Q_INVOKABLE void showInputPanel();
    Q_INVOKABLE void hideInputPanel();
    bool isInputPanelVisible() const { return mVisible; }

//    QLocale locale() const;
//    Qt::LayoutDirection inputDirection() const;
    void setFocusObject(QObject *object);

    // -- QML functions --
    void setVisible(bool value);
    void setAnimating(bool value);
    KeyboardType keyboardType() const { return mKeyboardType; }

    Q_INVOKABLE QString keyboardTypeString() const;
    Q_INVOKABLE void insertKey(const QString& text);
    Q_INVOKABLE void deleteKey();
    Q_INVOKABLE void setKeyboardRect(const QRectF& rect);

    // -- Test functions
public slots:
    void focusObjectChanged(QObject *focusObject);

signals:
    void visibleChanged();
    void animatingChanged();
    void keyboardTypeChanged();
    void show();
    void hide();

private:
    InputContext();
    void processInputMethodQuery(Qt::InputMethodQueries queries);

private:
    bool mVisible;
    bool mAnimating;
    QObject *mFocusObject;
    QRectF   mKeyboardRect;

    // Data extracted from Qt::InputMethodQueries
    KeyboardType mKeyboardType;
    int          mCursorPosition;
    int          mAnchorPosition;
};

QT_END_NAMESPACE

#endif
