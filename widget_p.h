#pragma once

#include <QObject>
#include <QList>
#include <QRadialGradient>

class Widget;
class QLCDNumber;
class QPushButton;
class QLabel;

class WidgetPrivate : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(WidgetPrivate)
    Q_DECLARE_PUBLIC(Widget)

protected:

    void timerEvent(QTimerEvent*) override;

private:

    enum class ValueAlphaEv : quint8
    {
        NONE = 0,
        DECR,
        DEFL
    };

    Widget* const q_ptr;
    QLCDNumber* value;
    QPushButton* start;
    QPushButton* stop;
    QLabel* correct;
    QLabel* correctCount;
    QLabel* incorrect;
    QLabel* incorrectCount;
    int timerOut, timerDiss, currentClicks;
    QList<std::pair<int, QRadialGradient>> waves; // очередь градиентов, "ключ" - id таймера

    explicit WidgetPrivate(Widget*);
    ~WidgetPrivate() override;

    void startGame();
    void stopGame();
    void incCounter(QLabel*);
    void updateValue();
    void repaintValueAlpha(ValueAlphaEv);
    void repaintGradient();

};
