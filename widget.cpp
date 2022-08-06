#include "widget.h"
#include "widget_p.h"
#include <QBoxLayout>
#include <QPainter>
#include <QMessageBox>
#include <QLCDNumber>
#include <QPushButton>
#include <QLabel>

static constexpr auto randInterval = 11;
static constexpr auto outInterval = std::chrono::seconds(2);
static constexpr auto dissInterval = std::chrono::milliseconds(40);
static constexpr auto decrCoeff = 0.999 * dissInterval / outInterval;
static constexpr auto clickSpreadTime = std::chrono::milliseconds(400);
static constexpr auto clickSpreadInterval = std::chrono::milliseconds(20);
static constexpr auto clickRadius = 40.;
static constexpr auto clickRadiusStep = clickRadius * clickSpreadInterval / clickSpreadTime;

Widget::Widget(QWidget* parent):
    QWidget(parent),
    d(new WidgetPrivate(this))
{
    auto buttonLayout = new QVBoxLayout();
    buttonLayout->addWidget(d->start);
    buttonLayout->addWidget(d->stop);

    auto countersLayout = new QGridLayout();
    countersLayout->addWidget(d->correct, 0, 0);
    countersLayout->addWidget(d->correctCount, 0, 1);
    countersLayout->addWidget(d->incorrect, 1, 0);
    countersLayout->addWidget(d->incorrectCount, 1, 1);

    auto bottomLayout = new QHBoxLayout();
    bottomLayout->addLayout(buttonLayout);
    bottomLayout->addLayout(countersLayout);

    auto mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(d->value);
    mainLayout->addLayout(bottomLayout);
}

Widget::~Widget()
{
    delete d;
}

void Widget::mouseReleaseEvent(QMouseEvent* event)
{
    // кликнута левая кнопка мыши во время игры
    if (d->timerOut != - 1 && event->button() == Qt::LeftButton) {
        QRadialGradient gradient(event->pos(), 0);
        gradient.setColorAt(0, Qt::transparent);
        gradient.setColorAt(1, Qt::cyan);
        d->waves.append({d->startTimer(clickSpreadInterval, Qt::PreciseTimer), gradient});
        ++(d->currentClicks);
    }
    QWidget::mouseReleaseEvent(event);
}

void Widget::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setPen(Qt::NoPen);
    for (const auto& wave : d->waves) {
        qreal radius = wave.second.radius();
        painter.setBrush(wave.second);
        painter.setOpacity(1 - radius / clickRadius); // больше радиус - меньше прозрачность
        painter.drawEllipse(wave.second.center(), radius, radius);
    }
    QWidget::paintEvent(event);
}

void Widget::closeEvent(QCloseEvent* event)
{
    d->stopGame();
    QMessageBox::information(this, "Уже уходите?", "Приходите еще!",
                             "Обязательно приду");
    QWidget::closeEvent(event);
}

void WidgetPrivate::timerEvent(QTimerEvent* event)
{
    auto timerId = event->timerId();
    if (timerId == timerDiss) { // пора "притушить" отображаемое число кликов
        repaintValueAlpha(ValueAlphaEv::DECR);
    }
    else if (timerId == timerOut) { // число кликов показывалось достаточно
        incCounter(currentClicks == value->intValue() ? correctCount : incorrectCount);
        updateValue();
        repaintValueAlpha(ValueAlphaEv::DEFL);
    }
    // пора обновить первый градиент из очереди
    else if (!waves.isEmpty() && waves.first().first == timerId) {
        repaintGradient();
    }
    QObject::timerEvent(event);
}

WidgetPrivate::WidgetPrivate(Widget* q):
    QObject(),
    q_ptr(q),
    value(new QLCDNumber(q)),
    start(new QPushButton("Старт", q)),
    stop(new QPushButton("Стоп", q)),
    correct(new QLabel("Верно:", q)),
    correctCount(new QLabel("0", q)),
    incorrect(new QLabel("Неверно:", q)),
    incorrectCount(new QLabel("0", q)),
    timerOut(- 1),
    timerDiss(- 1)
{
    value->setToolTip("Здесь будет показываться точное число кликов,\n"
                      "которое необходимо сделать левой кнопкой мыши");
    value->setDigitCount(0);
    value->setSegmentStyle(QLCDNumber::Flat);
    stop->setEnabled(false);
    correctCount->setStyleSheet(" border: 1px solid; font: bold");
    incorrectCount->setStyleSheet("border: 1px solid; font: bold");
    correct->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    correctCount->setAlignment(Qt::AlignCenter);
    incorrect->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    incorrectCount->setAlignment(Qt::AlignCenter);

    connect(start, &QPushButton::clicked, this, &WidgetPrivate::startGame);
    connect(stop,  &QPushButton::clicked, this, &WidgetPrivate::stopGame);
}

WidgetPrivate::~WidgetPrivate() = default;

void WidgetPrivate::startGame()
{
    value->setToolTipDuration(1);
    start->setEnabled(false);
    stop->setEnabled(true);
    correctCount->setText("0");
    incorrectCount->setText("0");
    updateValue();
    timerOut = startTimer(outInterval, Qt::PreciseTimer);
    timerDiss = startTimer(dissInterval, Qt::PreciseTimer);
}

void WidgetPrivate::stopGame()
{
    if (timerOut != - 1) { // игра идет
        killTimer(timerOut);
        killTimer(timerDiss);
        value->setToolTipDuration(- 1);
        value->setDigitCount(0);
        start->setEnabled(true);
        stop->setEnabled(false);
        timerOut = timerDiss = - 1;
        repaintValueAlpha(ValueAlphaEv::DEFL);
    }
}

void WidgetPrivate::incCounter(QLabel* counter)
{
    counter->setText(QString::number(counter->text().toInt() + 1));
}

void WidgetPrivate::updateValue()
{
    currentClicks = 0;
    int newValue = qrand() % (randInterval + 1);
    value->setDigitCount(newValue < 10 ? 1 : 2);
    value->display(newValue);
}

void WidgetPrivate::repaintValueAlpha(ValueAlphaEv valueAlphaEv)
{
    if (valueAlphaEv != ValueAlphaEv::NONE) {
        auto palette = value->palette();
        auto role = value->foregroundRole();
        auto color = palette.color(role);
        switch (valueAlphaEv) {
            case ValueAlphaEv::DECR:
                color.setAlphaF(color.alphaF() - decrCoeff);
                break;
            case ValueAlphaEv::DEFL:
                color.setAlpha(255);
                break;
            default:
                break;
        }
        palette.setColor(role, color);
        value->setPalette(palette);
        q_ptr->repaint();
    }
}

void WidgetPrivate::repaintGradient()
{
    auto& gradient = waves.first().second;
    qreal radius = gradient.radius();
    if (radius < clickRadius) { // "след" клика не слишком большой
        gradient.setRadius(radius + clickRadiusStep);
        waves.move(0, waves.count() - 1); // обработанный градиент в конец очереди
    }
    else {
        killTimer(waves.takeFirst().first); // остановка таймера градиента и удаление из очереди
    }
    q_ptr->repaint();
}
