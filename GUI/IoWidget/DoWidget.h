#ifndef DOWIDGET_H
#define DOWIDGET_H

#include <QWidget>

namespace Ui { class DoWidget; }

class DoWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DoWidget(int varId, const QString &name, QWidget *parent = nullptr);
    ~DoWidget();

    int varId() const { return m_varId; }
    void display(bool value);

signals:
    void setDo(int varId, bool value);

private:
    Ui::DoWidget *ui;
    int m_varId;
};

#endif // DOWIDGET_H
