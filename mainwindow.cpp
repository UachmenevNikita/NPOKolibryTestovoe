#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QApplication>
#include <QByteArray>
#include <QDebug>
#include <QDialog>
#include <QFile>
#include <QFileDialog>
#include <QRegularExpression>
#include <QString>
#include <qmath.h>
#include <qthread.h>
#include <qvalidator.h>
#include <xorconverter.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);
  this->setWindowTitle("Шифровщик");
  ui->lineEdit_bit->setMaxLength(8);
  QRegularExpression rx("[0,1]{8}");
  QRegularExpressionValidator *validator =
      new QRegularExpressionValidator(rx, this);
  ui->lineEdit_bit->setValidator(validator);
  ui->label_file_name->setHidden(1);
  ui->label_progress->setHidden(1);
  ui->progressBar->setHidden(1);
  ui->progressBar->setStyleSheet(
      "QProgressBar{"
      "text-align: center;"
      "}"
      "QProgressBar::chunk {background-color: green;}");
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::on_pushButton_input_clicked() {
  // выбор директории
  QString directory = QFileDialog::getExistingDirectory(
      this, "Выберите директорию", QDir::currentPath(),
      QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
  ui->lineEdit_input->setText(directory);
  qDebug() << directory;
}

void MainWindow::on_pushButton_start_clicked(bool checked) {
  // проверка на одинаковые дерриктории. проверка на наличие файлов. проверка на
  // выбор дерриктории.
  if (checked) {
    // проверка на заданность деррикторий
    if (ui->lineEdit_input->text().isEmpty()) {
      createDialog("Выберите директорию");
      ui->pushButton_start->setChecked(false);
      return;
    }
    if (ui->lineEdit_out->text().isEmpty()) {
      createDialog("Выберите директорию");
      ui->pushButton_start->setChecked(false);
      return;
    }
    // проверка на Одинаковость Директорий
    if (ui->lineEdit_input->text() == ui->lineEdit_out->text()) {
      createDialog("Выберите разные директории");
      ui->pushButton_start->setChecked(false);
      return;
    }
    // проверка на существование Директорий
    QString curdir = ui->lineEdit_input->text();
    QString desdir = ui->lineEdit_out->text();
    QDir dir(curdir);
    if (!dir.exists()) {
      createDialog("Входной директории не существует");
      ui->pushButton_start->setChecked(false);
      return;
    }
    dir.setPath(desdir);
    if (!dir.exists()) {
      createDialog("Выходной директории не существует");
      ui->pushButton_start->setChecked(false);
      return;
    }
    // проверка входной Директории на пустоту
    dir.setPath(curdir);
    if (dir.isEmpty(QDir::Files | QDir::NoDotAndDotDot)) {
      createDialog("Выходная директория пуста");
      ui->pushButton_start->setChecked(false);
      return;
    }
    // проверка бит маски
    if (ui->lineEdit_bit->text().length() != 8) {
      createDialog("Неверный формат бит маски");
      ui->pushButton_start->setChecked(false);
      return;
    }
    ui->progressBar->setStyleSheet(
        "QProgressBar{"
        "text-align: center;" // было right
        "}"
        "QProgressBar::chunk {background-color: green;}");
    ui->label_file_name->setHidden(0);
    ui->label_progress->setHidden(0);
    ui->progressBar->setHidden(0);
    ui->pushButton_start->setText("отмена");

    QString mask = ui->lineEdit_mask->text();
    if (!mask.isEmpty()) {
      if (mask[0] == '.') {
        mask.push_front('*');
      }
    }

    QString num = ui->lineEdit_bit->text();
    if (num.length() < 8) {
      qDebug() << "not enough l";
      return;
    }
    qDebug() << num;
    int inum = 0;

    for (int i = 7; i >= 0; i--) {
      if (num[i] == '1') {
        inum = inum + qPow(2, 7 - i);
      }
    }

    quint8 key = static_cast<quint8>(inum);

    workerThread = new QThread();

    xc = new XorConverter(curdir, desdir, key, mask,
                          ui->checkBox_del->isChecked());
    xc->moveToThread(workerThread);
    QObject::connect(workerThread, &QThread::started, xc,
                     &XorConverter::startWork);
    QObject::connect(xc, &XorConverter::finished, workerThread, &QThread::quit);
    QObject::connect(xc, &XorConverter::finished, xc,
                     &XorConverter::deleteLater);
    QObject::connect(workerThread, &QThread::finished, workerThread,
                     &QThread::deleteLater);

    QObject::connect(this, &MainWindow::stopWork, xc, &XorConverter::stopWork,
                     Qt::ConnectionType::DirectConnection);

    QObject::connect(xc, &XorConverter::progress, this,
                     [this](int value, QString name) {
                       ui->progressBar->setValue(value);
                       ui->label_file_name->setText(name);
                     });
    QObject::connect(xc, &XorConverter::finished, this, [this]() {
      ui->pushButton_start->setText("старт");
      ui->pushButton_start->setChecked(false);
      QDialog *readyWindow = new QDialog();
      readyWindow->setWindowTitle("Шифровщик");
      QVBoxLayout *layout = new QVBoxLayout(readyWindow);
      QLabel *lab = new QLabel("Операция завершена");
      QPushButton *ok = new QPushButton("ok");
      layout->addWidget(lab);
      layout->addWidget(ok);
      connect(ok, &QPushButton::clicked, readyWindow, &QDialog::close);
      connect(ok, &QPushButton::clicked, readyWindow, &QDialog::deleteLater);
      connect(readyWindow, &QDialog::finished, readyWindow,
              &QDialog::deleteLater);

      readyWindow->setWindowFlag(Qt::WindowStaysOnTopHint);
      readyWindow->setModal(true);
      readyWindow->show();
    });
    workerThread->start();
  } else {
    ui->pushButton_start->setText("старт");
    ui->progressBar->setStyleSheet(
        "QProgressBar{"
        "text-align: center;" // было right
        "}"
        "QProgressBar::chunk {background-color: red;}");

    emit stopWork();
  }
}

void MainWindow::on_pushButton_out_clicked() {
  QString directory = QFileDialog::getExistingDirectory(
      this, "Выберите директорию", QDir::currentPath(),
      QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
  ui->lineEdit_out->setText(directory);
  qDebug() << directory;
}

void MainWindow::createDialog(QString messege) {
  QDialog *readyWindow = new QDialog();
  readyWindow->setWindowTitle("Шифровщик");
  QVBoxLayout *layout = new QVBoxLayout(readyWindow);
  QLabel *lab = new QLabel(messege);
  QPushButton *ok = new QPushButton("ok");
  layout->addWidget(lab);
  layout->addWidget(ok);
  connect(ok, &QPushButton::clicked, readyWindow, &QDialog::close);
  connect(ok, &QPushButton::clicked, readyWindow, &QDialog::deleteLater);
  connect(readyWindow, &QDialog::finished, readyWindow, &QDialog::deleteLater);

  readyWindow->setWindowFlag(Qt::WindowStaysOnTopHint);
  readyWindow->setModal(true);
  readyWindow->show();
}
