/*QBedReader, a text to speech program for Wikipedia.
*Copyright (C) 2012 Kyle Plummer
*
*This program is free software; you can redistribute it and/or
*modify it under the terms of the GNU General Public License
*as published by the Free Software Foundation; either version 2
*of the License, or (at your option) any later version.
*
*This program is distributed in the hope that it will be useful,
*but WITHOUT ANY WARRANTY; without even the implied warranty of
*MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*GNU General Public License for more details.
*
*You should have received a copy of the GNU General Public License
*along with this program; if not, write to the Free Software
*Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*
*/
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
    void on_LinkList_itemActivated(QTreeWidgetItem *item);
    void ProcFinish();
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
