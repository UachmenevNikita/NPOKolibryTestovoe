#ifndef XORCONVERTER_H
#define XORCONVERTER_H
#include <QAtomicInteger>
#include <QByteArray>
#include <QCoreApplication>
#include <QDataStream>
#include <QDebug>
#include <QFile>
#include <QThread>

class XorConverter : public QObject {
  Q_OBJECT
public:
  XorConverter(const QString &InputDir, const QString &DesDir, quint8 XorKey,
               QString mask, bool df);
public slots:
  void startWork();
  void stopWork();

private:
  QString inputDir;
  QString desDir;
  quint8 xorKey;
  QString mask;
  bool df;
  bool infWork;
  QAtomicInteger<bool> stop = false;
  QStringList xorFiles;

  void copyFile(const QString &sourceFilePath,
                const QString &destinationFilePath, quint8 xorValue,
                qint64 bufferSize = 4096);
  void timerCheck();

signals:
  void progress(int value, QString name);
  void finished();
};

#endif // XORCONVERTER_H
