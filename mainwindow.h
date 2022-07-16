#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStack>
#include <task.h>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    static bool hasLocalFileInUrls(const QList<QUrl>&);

protected:
    void dragEnterEvent(QDragEnterEvent*);
    void dropEvent(QDropEvent*);

private:
    Ui::MainWindow *ui;
    QStack<Task> taskHistory;
    void newTask();
    void rename(Task::Mask);
    void setTaskView();

private slots:
    void changeDigitsByOrdinal(const QString&);
    void changeOrdinalByDigits(int);
    void enableRun(bool);
    void enableRunOrNot();
    void renameExtExcluded();
    void renameExtOnly();
    void switchToDelete(bool);
    void switchToInsert(bool);
};
#endif // MAINWINDOW_H
