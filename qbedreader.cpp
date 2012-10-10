#include "qbedreader.h"
#include "ui_qbedreader.h"

//On Windows we have flite as a local file, this indicates if we should check for it
bool WINDOWSBOOL;

QBedReader::QBedReader(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::QBedReader)
{
    ui->setupUi(this);
    connect(&manager,SIGNAL(finished(QNetworkReply*)),SLOT(DownloadFinished(QNetworkReply*))); //When the download finishes, DownloadFinished launches
    connect(ui->ClearLinkButton,SIGNAL(clicked()),ui->LinkList,SLOT(clear())); //Make the clear button directly clear the list.
    connect(&proc,SIGNAL(error(QProcess::ProcessError)),this,SLOT(ProcError(QProcess::ProcessError))); //If there is an error in flite, print it.
    connect(&proc,SIGNAL(finished(int,QProcess::ExitStatus)),this,SLOT(ProcFinish(int,QProcess::ExitStatus))); //When flite finishes. Used for autoreading
    srand(time(NULL));
    AutoReading = false; //If true the program starts reading the next article
    WikiURL = "http://en.wikipedia.org"; //Used for reading other wikis.
    WikiSubdir = "/wiki/"; //Some wikis might have a different subdirectory.
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
#ifdef Q_WS_WIN32 //If we're in windows
    Flite = "flite.exe";
    WINDOWSBOOL = true; 
#else
    Flite = "flite";
    WINDOWSBOOL = false;
#endif
}

QBedReader::~QBedReader()
{
    delete ui;
}

/*Slot for when the download finishes.
  Checks for redirects and if so, requests with the new URL
  Writes the URL to a cache file (so we don't have to request the same file twice.)
  Parses the cache file, or if it can't be written/read parses the reply.
  Removes the reply
  */
void QBedReader::DownloadFinished(QNetworkReply *reply)
{
    ui->RawContent->setText("Download Finished");
    OldUrl = RedirectURL(reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl(),OldUrl); //Check if we're redirected
    if(OldUrl.isEmpty()) //If we're not redirected
    {
        if(reply->error())
        {
            QMessageBox dlg;
            dlg.setText("There was an error downloading " + QString(reply->url().toEncoded().constData() )+  "\nError:" + reply->errorString());
            dlg.show();
        }
        else
        {
            QDir Temp = QDir::current(); //To check if a directory exists, we need the current dir
            if(!Temp.exists("Cache")) //If the cache dir doesn't exist
            {
                Temp.mkdir("Cache"); //Make it
            }
            QFile out("Cache/" + CurrentFilePath); //Cache files are in the "Cache" folder.
            if(out.open(QIODevice::WriteOnly))
            {
                out.write(reply->readAll());
                out.close();
                QFile in("Cache/" + CurrentFilePath);
                if(in.open(QIODevice::ReadOnly))
                {
                    ParseArticle(&in); //Parse Article takes a pointer, so we need to give the address
                    in.close();
                }
                else
                {
                    ui->RawContent->setText("Failed to open HTML from cache");
                    ParseArticle(reply);
                }
            }
            else
            {
                ui->RawContent->setText("Failed to write HTML to cache");
                ParseArticle(reply);
            }
        }
        if(AutoReading) //If we're autoreading, hit the read button.
            on_ReadContentButton_clicked();
    }
    else //If we're redirected
    {
        Download(OldUrl); //Download the new link
    }
    CurrentDownloads.removeAll(reply);
    reply->deleteLater();
}

//Checks for a redirect, and returns empty or the redirect
QUrl QBedReader::RedirectURL(const QUrl &possibleRedirectUrl, const QUrl &oldRedirectUrl) const
{
    QUrl Redirection;
    if(!possibleRedirectUrl.isEmpty() && possibleRedirectUrl != oldRedirectUrl)
        Redirection = possibleRedirectUrl;
    return Redirection;
}

//Checks if a cached version exists. If not, downloads it.
void QBedReader::Download(const QUrl &url)
{
    CurrentFilePath = SaveFileName(url);
    if(!QFile::exists(CurrentFilePath))
    {
        ui->RawContent->setText("Starting download");
        QNetworkRequest req(url);
        QNetworkReply *reply = manager.get(req);
        CurrentDownloads.append(reply);
    }
    else
    {
        QFile file(CurrentFilePath);
        if(!(file.open(QIODevice::ReadOnly)))
                return;
        ui->RawContent->setText(file.readAll());
        if(AutoReading)
            on_ReadContentButton_clicked();
    }
}

/*Checks if the replied page has a name.
If not it makes a unique name by appending a number to the word "download"
*/
QString QBedReader::SaveFileName(const QUrl &url)
{
    QString path = url.path();
    QString BaseName = QFileInfo(path).fileName();
    if(BaseName.isEmpty())
    {
        int i=2;
        BaseName = "download";
        while(QFile::exists(BaseName + ".html"))
        {
            BaseName += QString::number(i++);
            if(!QFile::exists(BaseName + ".html"))
                BaseName.chop(2);
        }
    }
    return BaseName + ".html";
}

/*Parses the text file
  Skips all text until the title
  Grabs all text inbetween tags
  <like>this</like>
  Grabs all links.
  If they're from the wiki, removes subdir
  Else writes the full URL
  Stops at the id "p-personal" or EOF
  Removes any unneeded formatting
  Writes parsed text to a file to be read
  */
bool QBedReader::ParseArticle(QIODevice *data)
{
    ui->RawContent->setText("Starting Parsing");
    QXmlStreamReader xml(data->readAll());
    QString Text, Link, Debug;
    while((!xml.atEnd()) && (xml.name().toString() != "title")) //Until we're at "title" skip text to reduce junk.
         xml.readNext();
    xml.readNext(); //When we're at the title tag we need to go forward one to get the actual text.
    Text = xml.text().toString();
    while(!xml.atEnd() && xml.name().toString() != "body") //Skip until the body to reduce junk
        xml.readNext();
    while(!xml.atEnd())
    {
        Debug = "";
        switch(xml.readNext())
        {
            case(QXmlStreamReader::Characters): {Debug += xml.text();
            Text += xml.text();break;} //If we're at characters, take the text
            case(QXmlStreamReader::StartElement):
            {
                if(xml.name().toString() == "a") //If it's a link
                {
                    Link = xml.attributes().value("href").toString(); //Save the link attribute to Link
                    if(Link.indexOf(WikiSubdir) != -1) //If Subdir is in Link
                    {
                        bool AddLink=true;
                        if(Link.indexOf(WikiSubdir) == 0) //If the beginning of the link is Subdir
                            Link.remove(0,WikiSubdir.size()); //Remove it
                        if(Link.indexOf("//") == 0) //If the beginning is //
                            Link.remove(0,2); //Remove it
                        Link = QUrl::fromPercentEncoding(QByteArray(Link.toUtf8()));
                        if(ui->actionClear_Duplicates->isChecked())
                        {
                            for(int i=0;i<ui->LinkList->topLevelItemCount();i++) //Check if the link is in the list
                            {
                                if(ui->LinkList->topLevelItem(i)->text(1) == Link)
                                {
                                    AddLink = false; //If so, set the flag so we don't double up
                                    break;
                                }
                            }
                        }
                        if(AddLink)
                        {
                            QTreeWidgetItem *itm = new QTreeWidgetItem(ui->LinkList); //We need a new item to add it
                            itm->setText(0,"0"); //The default read value is 0
                            itm->setText(1,Link); //This takes the link's value and removes the percent signs
                        }
                    }
                }
                else if(xml.attributes().value("id") == "p-personal") //Wikipedia's actual content ends when this id comes up.
                    while(!xml.atEnd()) //If we're there, we can skip through the rest of the text.
                        xml.readNext();
            }
        default:
            break;
        }
    }
    try //On other wikis sometimes the text isn't all there, so RemoveJunk throws. If we don't catch it, we crash.
    {
        QString Output = RemoveJunk(Text); //Remove excess formatting (citations & excess newlines)
        QFile TalkToMe("Cache/TalkToMe.txt"); //The file read by the TTS
        if(TalkToMe.open(QIODevice::WriteOnly))
        {
            QTextStream os(&TalkToMe);
            os << Output;
            TalkToMe.close();
            ui->RawContent->setText("Setting Text");
            ui->RawContent->setText(Output);
        }
        else
        {
            ui->RawContent->setText("Failed to write to TTS file");
            return false;
        }
        return true;
    }
    catch(...)
    {
        ui->RawContent->setText("The website didn't like us. Try another or go back to Wikipedia");
        return false;
    }
}
//Removes excess formatting (citations & excess newlines)
inline QString QBedReader::RemoveJunk(QString Text)
{
    ui->RawContent->setText("Starting Junk Removal");
    QString Output;
    if(Text.size() < 1) //If we have no text to modify, throw a random number
        throw 20;
    Output += Text.at(0); //Due to using length()-1, we need to grab the first character.
    bool InSquare=false; //If we're in square brackets (like [2]) we set this.
    for(int i=1;i<Text.length();i++)
    {
        if(Text.at(i) == '[') //If we start a square bracket we set the flag
            InSquare = true;
        else if(!InSquare)
        {
            if(!(Output.at(Output.length()-1) == '\n' && Text.at(i) == '\n')) //If two newlines don't touch
                if(!(Output.at(Output.length()-1) == '\n' && Text.at(i) == '\t')) //And newline and tab don't touch
                    Output += Text.at(i); //Append the letter
        }
        else if(Text.at(i) == ']') //If we're at the end of the square bracket, we unset the flag
            InSquare = false;
    }
    return(Output);
}

//Starts the autoreader and gots to ProcFinished
void QBedReader::on_MainButton_clicked()
{
    if(AutoReading) //If we're reading and the button is pushed
    {
        proc.kill(); //Kill flite
        AutoReading = false; //Unset the flag
        ui->RawContent->setText("Killing AutoReader");
    }
    else
    {
        ui->RawContent->setText("Starting AutoReader");
        AutoReading = true;
        ProcFinish(); //This is the point the loop returns to. For reuse it's best to go here manually
    }
}
//Saves the links and Wiki used to a user specified file
void QBedReader::on_SaveLinksButton_clicked()
{
    QFileDialog dlg(this);
    dlg.setFileMode(QFileDialog::AnyFile);
    QFile file(dlg.getSaveFileName(this,"Save Links","",tr("Plaintext (*.txt);;All files(*.*)")));
    if(!file.open(QIODevice::WriteOnly))
        return;
    QTextStream out(&file);
    out.setCodec("UTF-8");
    out << WikiURL << "||" << WikiSubdir << '\n'; //We write the URL and subdir of the wiki. As well as a deliminator for them
    for(int i=0;i<ui->LinkList->topLevelItemCount();i++)
    {
        //Gets the read count and link URL
        out << ui->LinkList->topLevelItem(i)->text(0) << "||" << QUrl::toPercentEncoding(ui->LinkList->topLevelItem(i)->text(1)) << '\n';
    }
    file.close();
}
//Loads the links and Wiki used from a user specified file
void QBedReader::on_LoadLinkButton_clicked()
{
    QFileDialog dlg(this);
    QFile file(dlg.getOpenFileName(this,"Save Links","",tr("Plaintext (*.txt);;All files(*.*)")));
    if(!file.open(QIODevice::ReadOnly))
        return;
    WikiSubdir = file.readLine();
    WikiURL = "";
    for(int i=0;WikiSubdir.at(i) != '|';i++) //If we're not at the delim
    {
        WikiURL += WikiSubdir.at(i);
    }
    WikiSubdir.remove(WikiURL+"||",Qt::CaseSensitive).chop(1); //Remove the Wiki and delim, as well as the newline to get the subdir
    while(!file.atEnd())
    {
        QString temp = file.readLine(), Link;
        int ReadCount=0;
        bool AtLink=false; //True if we're at the link, false if we're at the read count
        for(int i=0;i<temp.length();i++)
        {
            if(!AtLink)
            {
                if(temp.at(i) != '|') //If we're not at the delim
                {
                    ReadCount *= 10; //Times the current read count by ten
                    ReadCount += temp.at(i).toAscii() - '0'; //And add the new number. - '0' to get an int from a char
                }
                else if(temp.at(i+1) == '|')
                {
                    i++;
                    AtLink=true;
                }
            }
            else if(temp.at(i) != '\n')
                Link += temp.at(i);
        }
        QTreeWidgetItem *itm = new QTreeWidgetItem(ui->LinkList); //To add to the list, we need a new widget item
        itm->setText(0,QString::number(ReadCount)); //The number of read items is in column 0
        itm->setText(1,QUrl::fromPercentEncoding(QByteArray(Link.toUtf8()))); //The link is in column 1
    }
}
/*When a list item is activated
  Write the URL/Filename to the URL field
  Push the fetch button
  */
void QBedReader::on_LinkList_itemActivated(QTreeWidgetItem *item)
{
    ui->RawURLText->setText(item->text(1));//Write the URL/Filename to the URL field
    on_FetchButton_clicked(); //Push the fetch button
    item->setText(0,QString::number(item->text(0).toInt() + 1));//Increase the read count by 1
}
//Saves the content to be read by the TTS, if the user wants.
void QBedReader::on_SaveContentButton_clicked()
{
    QFileDialog dlg(this);
    dlg.setFileMode(QFileDialog::AnyFile);
    QFile file(dlg.getSaveFileName(this,"Save Text","",tr("Plaintext (*.txt);;All files(*.*)")));
    if(!file.open(QIODevice::WriteOnly))
        return;
    QTextStream out(&file);
    out.setCodec("UTF-8");
    out << ui->RawContent->toPlainText();
    file.close();
}
/*Fetches the page requested
  Checks if there's content in the URL/search field
  If not, check if there's something in the link list unread
  If so, activate it.
  If not, picks a number and gets the article for it
  If there's something in the field, check if it's a URL
  If it's a URL download it
  Else put the Wiki URL and subdir in and download it
  */
void QBedReader::on_FetchButton_clicked()
{
    ui->RawContent->setText("Fetching Link");
    if(!ui->RawURLText->text().isEmpty()) //If there's content in the URL/search field
    {
        if(ui->RawURLText->text().indexOf("http://") != -1) //Check if it's a URL
        {
            QUrl url(ui->RawURLText->text());
            WikiSubdir = url.path(); //Set the new page's subdir to the path
            WikiURL = url.toString(); //Set the page's URL to the URL given
            WikiURL.remove(WikiSubdir); //Without the subdir
            while(WikiSubdir.at(WikiSubdir.length()-1) != '/')
                WikiSubdir.chop(1); //Remove everything up to the last '/'
            Download(url);
        }
        else
        {
            //If it's not a URL, attach the URL and subdir and download it.
            Download(QUrl(WikiURL + WikiSubdir + ui->RawURLText->text()));
        }
    }
    else //If there's no content
        if(ui->LinkList->topLevelItemCount()) //If there's content in the list
        {
            if(ui->LinkList->selectedItems().isEmpty()) //If there's no selected items
            {
                int GiveUpCount=ui->LinkList->topLevelItemCount()*100; //Pick an arbitrary number to give up after
                if(GiveUpCount < 0) //Overflow check
                    GiveUpCount=ui->LinkList->topLevelItemCount(); //Fallback for if we've overflowed
                while(GiveUpCount--)//Until we give up
                {
                    //Pick a random link from the list
                    QTreeWidgetItem *itm = ui->LinkList->topLevelItem(rand() % ui->LinkList->topLevelItemCount());
                    if(!itm->text(0).toInt()) //If it's unread
                    {
                        on_LinkList_itemActivated(itm); //Activate it
                        break;
                    }
                }
            }
            else //Else pick the first.
            {
                on_LinkList_itemActivated(ui->LinkList->selectedItems().at(0));
                ui->LinkList->selectedItems().at(0)->setSelected(false);
            }
        }
        else //If there's also no link, pick a number from 1-100 and pull it up
        {
            Download(QUrl(WikiURL + WikiSubdir + QString::number(rand() % 100)));
        }
    ui->RawURLText->setText("");
}
//Reports errors that flite could produce
void QBedReader::ProcError(QProcess::ProcessError err)
{
    switch(err)
    {
    case QProcess::FailedToStart:
            QMessageBox::information(0,"FailedToStart","FailedToStart");
            break;
    case QProcess::Crashed: //Often caused when killed. TODO: fix that
            QMessageBox::information(0,"Crashed","Crashed");
            break;
    case QProcess::Timedout:
            QMessageBox::information(0,"FailedToStart","FailedToStart");
            break;
    case QProcess::WriteError:
            QMessageBox::information(0,"Timedout","Timedout");
            break;
    case QProcess::ReadError:
            QMessageBox::information(0,"ReadError","ReadError");
            break;
    case QProcess::UnknownError:
            QMessageBox::information(0,"UnknownError","UnknownError");
            break;
    default:
            QMessageBox::information(0,"default","default");
            break;
    }
}

//Calls the program and gives it the text to read
void QBedReader::on_ReadContentButton_clicked()
{
    //If we're not in windows we only need to check for the TTS file's existence
    if((!WINDOWSBOOL || QFile::exists("flite.exe")) && QFile::exists("Cache/TalkToMe.txt"))
    {
        if(proc.state() == QProcess::Running)
            proc.kill();
        proc.start(Flite + " -f \"" + "Cache/TalkToMe.txt" + "\"");
        proc.waitForStarted(); //Don't leave the function until it's started
    }
    else
        QMessageBox::information(0,"Could not find flite and/or file","Could not find flite and/or file");
}

//When flite finishes (or manually), this is called
void QBedReader::ProcFinish()
{
    ui->RawContent->setText("Looping AutoReader");
    if(AutoReading)
    {
        //If we're in windows, check if flite.exe exists
        if((!WINDOWSBOOL || QFile::exists("flite.exe")))
        {
            on_FetchButton_clicked(); //Hit the fetch button
            ui->RawURLText->clear();
        }
        else //If not, stop autoreading
        {
            QMessageBox::information(0,"Couldn't find flite","Could not find flite");
            AutoReading = false;
        }
    }
    else
        ui->RawContent->setText("Not Looping");
}
