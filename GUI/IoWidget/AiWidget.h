#ifndef AIWIDGET_H
#define AIWIDGET_H

#include <QWidget>

namespace Ui { class AiWidget; }

class AiWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AiWidget(int varId, const QString &name, QWidget *parent = nullptr);
    ~AiWidget();

    int varId() const { return m_varId; }
    void display(double value);

private:
    Ui::AiWidget *ui;
    int m_varId;
};

#endif // AIWIDGET_H
