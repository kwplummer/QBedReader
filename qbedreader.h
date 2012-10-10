#ifndef _QBEDREADER_H
#define _QBEDREADER_H
#include <QMainWindow>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QList>
#include <QFileInfo>
#include <QMessageBox>
#include <QXmlStreamReader>
#include <QFileDialog>
#include <QTreeWidgetItem>
#include <QTextStream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <QProcess>
#include <QTextCodec>

namespace Ui {
    class QBedReader;
}

class QBedReader : public QMainWindow
{
    Q_OBJECT
public:
    explicit QBedReader(QWidget *parent = 0);
    ~QBedReader();
    void Download(const QUrl &url);
    QString SaveFileName(const QUrl &url);
    bool ParseArticle(QIODevice *data);
private slots:
    void DownloadFinished(QNetworkReply* reply);
    void on_MainButton_clicked();
    void on_SaveLinksButton_clicked();
    void ProcError(QProcess::ProcessError err);
    void on_LoadLinkButton_clicked();
    void on_LinkList_itemActivated(QTreeWidgetItem *item, int column);
    void ProcFinish(int,QProcess::ExitStatus);
    void on_SaveContentButton_clicked();
    void on_FetchButton_clicked();
    void on_ReadContentButton_clicked();
private:
    QUrl RedirectURL(const QUrl& possibleRedirectUrl, const QUrl& oldRedirectUrl) const;
    QUrl OldUrl;
    QString RemoveJunk(QString Text);
    Ui::QBedReader *ui;
    QNetworkAccessManager manager;
    QList<QNetworkReply*> CurrentDownloads;
    QString CurrentFilePath;
    QProcess proc;
    QString Flite;
    QString WikiURL;
    QString WikiSubdir;
    bool AutoReading;
};

#endif // _QBEDREADER_H
