#ifndef AOWIDGET_H
#define AOWIDGET_H

#include <QWidget>

namespace Ui { class AoWidget; }

class AoWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AoWidget(int varId, const QString &name, QWidget *parent = nullptr);
    ~AoWidget();

    int varId() const { return m_varId; }
    void display(double currentValue);

signals:
    void setAo(int varId, double value);

private:
    Ui::AoWidget *ui;
    int m_varId;
};

#endif // AOWIDGET_H
