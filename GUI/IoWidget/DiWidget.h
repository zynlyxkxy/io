#ifndef DIWIDGET_H
#define DIWIDGET_H

#include <QWidget>

namespace Ui { class DiWidget; }

class DiWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DiWidget(int varId, const QString &name, QWidget *parent = nullptr);
    ~DiWidget();

    int varId() const { return m_varId; }
    void display(bool value);

private:
    Ui::DiWidget *ui;
    int m_varId;
};

#endif // DIWIDGET_H
