/***************************************************************************
 *   Copyright (C) 2011 by Jean-Baptiste Mardelle (jb@kdenlive.org)        *
 *                                                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA          *
 ***************************************************************************/


#include "freesound.h"

#include <QPushButton>
#include <QListWidget>
#include <QApplication>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QDebug>

#include "kdenlivesettings.h"
#include <kio/storedtransferjob.h>
#include <KLocalizedString>
#include <KMessageBox>

FreeSound::FreeSound(QListWidget *listWidget, QObject *parent) :
        AbstractService(listWidget, parent),
        m_previewProcess(new QProcess)
{
    serviceType = FREESOUND;
    hasPreview = true;
    hasMetadata = true;
}

FreeSound::~FreeSound()
{
    delete m_previewProcess;
}

void FreeSound::slotStartSearch(const QString &searchText, int page)
{
    m_listWidget->clear();
    QString uri = "http://www.freesound.org/api/sounds/search/?q=";
    uri.append(searchText);
    if (page > 1)
        uri.append("&p=" + QString::number(page));
    uri.append("&api_key=a1772c8236e945a4bee30a64058dabf8");

    
    KIO::StoredTransferJob* resolveJob = KIO::storedGet( QUrl(uri), KIO::NoReload, KIO::HideProgressInfo );
    connect(resolveJob, &KIO::StoredTransferJob::result, this, &FreeSound::slotShowResults);
}


void FreeSound::slotShowResults(KJob* job)
{
    if (job->error() != 0 ) return;
    m_listWidget->blockSignals(true);
    KIO::StoredTransferJob* storedQueryJob = static_cast<KIO::StoredTransferJob*>( job );
    ////qDebug()<<"// GOT RESULT: "<<m_result;
    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(storedQueryJob->data(), &jsonError);
    if (jsonError.error != QJsonParseError::NoError) {
        // There was an error parsing data
        KMessageBox::sorry(m_listWidget, jsonError.errorString(), i18n("Error Loading Data"));
        
    }
    QVariant data = doc.toVariant();
    QVariant sounds;
    if (data.canConvert(QVariant::Map)) {
        QMap <QString, QVariant> map = data.toMap();
        QMap<QString, QVariant>::const_iterator i = map.constBegin();
        while (i != map.constEnd()) {
            if (i.key() == "num_results") emit searchInfo(i18np("Found %1 result", "Found %1 results", i.value().toInt()));
            else if (i.key() == "num_pages") {
                emit maxPages(i.value().toInt());
            }
            else if (i.key() == "sounds") {
                sounds = i.value();
                if (sounds.canConvert(QVariant::List)) {
                    QList <QVariant> soundsList = sounds.toList();
                    for (int j = 0; j < soundsList.count(); ++j) {
                        if (soundsList.at(j).canConvert(QVariant::Map)) {
                            QMap <QString, QVariant> soundmap = soundsList.at(j).toMap();
                            if (soundmap.contains("original_filename")) {
                                QListWidgetItem *item = new   QListWidgetItem(soundmap.value("original_filename").toString(), m_listWidget);
                                item->setData(imageRole, soundmap.value("waveform_m").toString());
                                item->setData(infoUrl, soundmap.value("url").toString());
                                item->setData(infoData, soundmap.value("ref").toString() + "?api_key=a1772c8236e945a4bee30a64058dabf8");
                                item->setData(durationRole, soundmap.value("duration").toDouble());
                                item->setData(idRole, soundmap.value("id").toInt());
                                item->setData(previewRole, soundmap.value("preview-hq-mp3").toString());
                                item->setData(downloadRole, soundmap.value("serve").toString() + "?api_key=a1772c8236e945a4bee30a64058dabf8");
                                QVariant authorInfo = soundmap.value("user");
                                if (authorInfo.canConvert(QVariant::Map)) {
                                    QMap <QString, QVariant> authorMap = authorInfo.toMap();
                                    if (authorMap.contains("username")) {
                                        item->setData(authorRole, authorMap.value("username").toString());
                                        item->setData(authorUrl, authorMap.value("url").toString());
                                    }
                                }
                            }
                        }
                    }
                }
            }
            ++i;
        }
    }
    m_listWidget->blockSignals(false);
    m_listWidget->setCurrentRow(0);
    emit searchDone();
}
    

OnlineItemInfo FreeSound::displayItemDetails(QListWidgetItem *item)
{
    OnlineItemInfo info;
    m_metaInfo.clear();
    if (!item) {
        return info;
    }
    info.itemPreview = item->data(previewRole).toString();
    info.itemDownload = item->data(downloadRole).toString();
    info.itemId = item->data(idRole).toInt();
    info.itemName = item->text();
    info.infoUrl = item->data(infoUrl).toString();
    info.author = item->data(authorRole).toString();
    info.authorUrl = item->data(authorUrl).toString();
    m_metaInfo.insert(i18n("Duration"), item->data(durationRole).toString());
    info.license = item->data(licenseRole).toString();
    info.description = item->data(descriptionRole).toString();
    
    QString extraInfoUrl = item->data(infoData).toString();
    if (!extraInfoUrl.isEmpty()) {
        KJob* resolveJob = KIO::storedGet( QUrl(extraInfoUrl), KIO::NoReload, KIO::HideProgressInfo );
        connect(resolveJob, &KJob::result, this, &FreeSound::slotParseResults);
    }
    emit gotThumb(item->data(imageRole).toString());
    return info;
}


void FreeSound::slotParseResults(KJob* job)
{
#ifdef USE_QJSON
    KIO::StoredTransferJob* storedQueryJob = static_cast<KIO::StoredTransferJob*>( job );
    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(storedQueryJob->data(), &jsonError);
    if (jsonError.error != QJsonParseError::NoError) {
        // There was an error parsing data
        KMessageBox::sorry(m_listWidget, jsonError.errorString(), i18n("Error Loading Data"));
    }
    QVariant data = doc.toVariant();
    QString html = QString("<style type=\"text/css\">tr.cellone {background-color: %1;}</style>").arg(qApp->palette().alternateBase().color().name());
    if (data.canConvert(QVariant::Map)) {
        QMap <QString, QVariant> info = data.toMap();
        //if (m_currentId != info.value("id").toInt()) return;
        
        html += "<table width=\"100%\" cellspacing=\"0\" cellpadding=\"2\">";
    
        if (m_metaInfo.contains(i18n("Duration"))) {
	    html += "<tr>";
            html += "<td>" + i18n("Duration") + "</td><td>" + m_metaInfo.value(i18n("Duration")) + "</td></tr>";
            m_metaInfo.remove(i18n("Duration"));
        }
        
        if (info.contains("samplerate")) {
            html += "<tr class=\"cellone\">";
            html += "<td>" + i18n("Samplerate") + "</td><td>" + QString::number(info.value("samplerate").toDouble()) + "</td></tr>";
        }
        if (info.contains("channels")) {
            html += "<tr>";
            html += "<td>" + i18n("Channels") + "</td><td>" + QString::number(info.value("channels").toInt()) + "</td></tr>";
        }
        if (info.contains("filesize")) {
            html += "<tr class=\"cellone\">";
            KIO::filesize_t fSize = info.value("filesize").toDouble();
            html += "<td>" + i18n("File size") + "</td><td>" + KIO::convertSize(fSize) + "</td></tr>";
        }
        if (info.contains("license")) {
            m_metaInfo.insert("license", info.value("license").toString());
        }
        html +="</table>";
        if (info.contains("description")) {
            m_metaInfo.insert("description", info.value("description").toString());
        }
    }
    emit gotMetaInfo(html);
    emit gotMetaInfo(m_metaInfo);
#else
	Q_UNUSED(job)
#endif    
}


bool FreeSound::startItemPreview(QListWidgetItem *item)
{    
    if (!item)
        return false;
    const QString url = item->data(previewRole).toString();
    if (url.isEmpty())
        return false;
    if (m_previewProcess) {
	    if (m_previewProcess->state() != QProcess::NotRunning) {
		    m_previewProcess->close();
		}
		m_previewProcess->start(KdenliveSettings::ffplaypath(), QStringList() << url << "-nodisp");
	}
    return true;
}


void FreeSound::stopItemPreview(QListWidgetItem * /*item*/)
{    
    if (m_previewProcess && m_previewProcess->state() != QProcess::NotRunning) {
        m_previewProcess->close();
    }
}

QString FreeSound::getExtension(QListWidgetItem *item)
{
    if (!item)
        return QString();
    return QString("*.") + item->text().section('.', -1);
}


QString FreeSound::getDefaultDownloadName(QListWidgetItem *item)
{
    if (!item)
        return QString();
    return item->text();
}


