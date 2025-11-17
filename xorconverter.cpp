#include "xorconverter.h"
#include <qdir.h>

XorConverter::XorConverter(const QString &InputDir, const QString &DesDir,
                           quint8 XorKey, QString mask, bool df)
    : inputDir(InputDir), desDir(DesDir), xorKey(XorKey), mask(mask), df(df) {}

void XorConverter::startWork() {

  QDir dir(inputDir);
  QStringList entries;

  if (!mask.isEmpty()) {
    entries = dir.entryList(QStringList() << mask,
                            QDir::Files | QDir::NoDotAndDotDot);
  } else {
    entries = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
  }

  for (int i = 0; i < entries.length(); i++) {
    if (stop.loadRelaxed()) {
      emit finished();
      return;
    }
    QFileInfo fileInfo(inputDir + '/' + entries[i]);
    qDebug() << inputDir + '/' + entries[i];
    if (!fileInfo.exists()) {
      continue;
    }
    if (fileInfo.isDir()) {
      continue;
    }
    qDebug() << fileInfo.size();
    copyFile(inputDir + '/' + entries[i], desDir + '/' + entries[i], xorKey);
  }

  emit finished();
}

void XorConverter::stopWork() { stop.storeRelaxed(true); }

void XorConverter::copyFile(const QString &sourceFilePath,
                            const QString &destinationFilePath, quint8 xorValue,
                            qint64 bufferSize) {
  QFile sourceFile(sourceFilePath);
  QFile destinationFile(destinationFilePath);

  // Открываем исходный файл для чтения
  if (!sourceFile.open(QIODevice::ReadOnly)) {
    qWarning() << "Не удалось открыть файл" << sourceFilePath << "для чтения.";
    return;
  }

  // Открываем целевой файл для записи
  if (!destinationFile.open(QIODevice::WriteOnly)) {
    qWarning() << "Не удалось открыть файл" << destinationFilePath
               << "для записи.";
    sourceFile.close(); // Закрываем исходный файл при ошибке
    return;
  }

  // Буфер для чтения данных
  QByteArray buffer;
  emit progress(0, "");

  // Чтение и запись данных
  while (!sourceFile.atEnd()) {
    if (stop.loadRelaxed()) {
      return;
    }
    buffer = sourceFile.read(bufferSize); // Считываем данные порциями
    for (int i = 0; i < buffer.size(); ++i) {
      buffer[i] ^= xorValue; // Проводим операцию XOR
    }
    destinationFile.write(buffer); // Записываем считанные данные в целевой файл
    // qDebug()<<destinationFile.size();
    // long progres=((long)destinationFile.size()/(long)sourceFile.size())*100;
    double a = destinationFile.size();
    double b = sourceFile.size();
    double progres = (a / b) * 100;
    emit progress((int)progres, sourceFile.fileName());
  }

  // Закрываем файлы
  if (df) {
    sourceFile.remove();
  }
  sourceFile.close();
  destinationFile.close();

  qDebug() << "Файл" << sourceFilePath << "успешно скопирован в"
           << destinationFilePath;
}
