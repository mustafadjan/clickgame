#pragma once

#include <QWidget>

class WidgetPrivate;

class Widget : public QWidget
{
    Q_OBJECT
    //Q_DECLARE_PRIVATE_D(d, Widget)

public:

    explicit Widget(QWidget* = nullptr);
    ~Widget() override;

protected:

    void mouseReleaseEvent(QMouseEvent*) override;
    void paintEvent(QPaintEvent*) override;
    void closeEvent(QCloseEvent*) override;

private:

    WidgetPrivate* const d;

};
