
#include <qapplication.h>
#include <qdesktopwidget.h>
#include <qevent.h>
#include <qhash.h>
#include <qlabel.h>
#include <qpointer.h>
#include <qstyle.h>
#include <qstyleoption.h>
#include <qstylepainter.h>
#include <qtimer.h>
#include <qtooltip.h>
#include <private/qeffects_p.h>
#include <qdebug.h>

#include "tooltip.h"

class TipLabel : public QLabel
{
    Q_OBJECT
public:
    TipLabel(const QString& text, QWidget* parent);
    ~TipLabel();
    static TipLabel *instance;

    bool eventFilter(QObject *, QEvent *);

    QBasicTimer hideTimer, deleteTimer;

    void hideTip();
protected:
    void enterEvent(QEvent*){hideTip();}
    void timerEvent(QTimerEvent *e);
    void paintEvent(QPaintEvent *e);

};

TipLabel *TipLabel::instance = 0;

TipLabel::TipLabel(const QString& text, QWidget* parent)
    : QLabel(parent, Qt::ToolTip)
{
    delete instance;
    instance = this;
    setMargin(1 + style()->pixelMetric(QStyle::PM_ToolTipLabelFrameWidth, 0, this));
    setFrameStyle(QFrame::NoFrame);
    setAlignment(Qt::AlignLeft);
    setIndent(1);
    ensurePolished();
    setText(text);

    QFontMetrics fm(font());
    QSize extra(1, 0);
    // Make it look good with the default ToolTip font on Mac, which has a small descent.
    if (fm.descent() == 2 && fm.ascent() >= 11)
        ++extra.rheight();

    resize(sizeHint() + extra);
    qApp->installEventFilter(this);
    hideTimer.start(10000, this);
    setWindowOpacity(style()->styleHint(QStyle::SH_ToolTipLabel_Opacity, 0, this) / 255.0);
    // No resources for this yet (unlike on Windows).
    QPalette pal(Qt::black, QColor(255,255,220),
                  QColor(96,96,96), Qt::black, Qt::black,
                  Qt::black, QColor(255,255,220));
    setPalette(pal);
}

void TipLabel::paintEvent(QPaintEvent *ev)
{
    QStylePainter p(this);
    QStyleOptionFrame opt;
    opt.init(this);
    p.drawPrimitive(QStyle::PE_PanelTipLabel, opt);
    p.end();

    QLabel::paintEvent(ev);
}

TipLabel::~TipLabel()
{
    instance = 0;
}

void TipLabel::hideTip()
{
    hide();
    // timer based deletion to prevent animation
    deleteTimer.start(250, this);
}

void TipLabel::timerEvent(QTimerEvent *e)
{
    if (e->timerId() == hideTimer.timerId())
        hideTip();
    else if (e->timerId() == deleteTimer.timerId())
        delete this;
}

bool TipLabel::eventFilter(QObject *, QEvent *e)
{
    switch (e->type()) {
    case QEvent::KeyPress:
    case QEvent::KeyRelease: {
        int key = static_cast<QKeyEvent *>(e)->key();
        Qt::KeyboardModifiers mody = static_cast<QKeyEvent *>(e)->modifiers();

        if ((mody & Qt::KeyboardModifierMask)
            || (key == Qt::Key_Shift || key == Qt::Key_Control
                || key == Qt::Key_Alt || key == Qt::Key_Meta))
            break;
    }
    case QEvent::Leave:
    case QEvent::WindowActivate:
    case QEvent::WindowDeactivate:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::FocusIn:
    case QEvent::FocusOut:
        hideTip();
    default:
        ;
    }
    return false;
}

/*!
    Shows \a text as a tool tip, at global position \a pos. The
    optional widget argument, \a w, is used to determine the
    appropriate screen on multi-head systems.
*/
void ToolTip::showText(const QPoint &pos, const QString &text, QWidget *w)
{
    if (TipLabel::instance && TipLabel::instance->text() == text)
        return;

    if (text.isEmpty()) {
        if (TipLabel::instance)
            TipLabel::instance->hideTip();
        return;
    }

    bool preventAnimation = (TipLabel::instance != 0);
    int scr;
    if (QApplication::desktop()->isVirtualDesktop())
        scr = QApplication::desktop()->screenNumber(pos);
    else
        scr = QApplication::desktop()->screenNumber(w);

#ifdef Q_WS_MAC
    QRect screen = QApplication::desktop()->availableGeometry(scr);
#else
    QRect screen = QApplication::desktop()->screenGeometry(scr);
#endif

    QLabel *label = new TipLabel(text, QApplication::desktop()->screen(scr));

    QPoint p = pos;
    p += QPoint(2,
#ifdef Q_WS_WIN
                24
#else
                16
#endif
        );
    if (p.x() + label->width() > screen.x() + screen.width())
        p.rx() -= 4 + label->width();
    if (p.y() + label->height() > screen.y() + screen.height())
        p.ry() -= 24 + label->height();
    if (p.y() < screen.y())
        p.setY(screen.y());
    if (p.x() + label->width() > screen.x() + screen.width())
        p.setX(screen.x() + screen.width() - label->width());
    if (p.x() < screen.x())
        p.setX(screen.x());
    if (p.y() + label->height() > screen.y() + screen.height())
        p.setY(screen.y() + screen.height() - label->height());
    label->move(p);

#ifndef QT_NO_EFFECTS
    if ( QApplication::isEffectEnabled(Qt::UI_AnimateTooltip) == false || preventAnimation)
        label->show();
    else if (QApplication::isEffectEnabled(Qt::UI_FadeTooltip)) {
        qFadeEffect(label);
    }
    else
        qScrollEffect(label);
#else
    label->show();
#endif

}

/*!
    Returns the palette used to render tooltips.
*/
QPalette ToolTip::palette()
{
    if (TipLabel::instance)
        return TipLabel::instance->palette();
    return QPalette(Qt::black, QColor(255,255,220),
                    QColor(96,96,96), Qt::black, Qt::black,
                    Qt::black, QColor(255,255,220));
}
