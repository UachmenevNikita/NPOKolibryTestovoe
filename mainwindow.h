#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QApplication>
#include <QMainWindow>
#include <xorconverter.h>
QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

private slots:
  void on_pushButton_input_clicked();
  void on_pushButton_start_clicked(bool checked);
  void on_pushButton_out_clicked();

signals:
  void stopWork();

private:
  Ui::MainWindow *ui;
  XorConverter *xc = nullptr;
  QThread *workerThread = nullptr;
  void createDialog(QString messege);
};
#endif // MAINWINDOW_H
