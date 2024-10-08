//////////////////////////////////////////////////////////////////////////////////////
//    akashi - a server for Attorney Online 2                                       //
//    Copyright (C) 2020  scatterflower                                             //
//                                                                                  //
//    This program is free software: you can redistribute it and/or modify          //
//    it under the terms of the GNU Affero General Public License as                //
//    published by the Free Software Foundation, either version 3 of the            //
//    License, or (at your option) any later version.                               //
//                                                                                  //
//    This program is distributed in the hope that it will be useful,               //
//    but WITHOUT ANY WARRANTY; without even the implied warranty of                //
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 //
//    GNU Affero General Public License for more details.                           //
//                                                                                  //
//    You should have received a copy of the GNU Affero General Public License      //
//    along with this program.  If not, see <https://www.gnu.org/licenses/>.        //
//////////////////////////////////////////////////////////////////////////////////////
#include "logger/writer_full.h"

WriterFull::WriterFull(QObject *parent) :
    QObject(parent)
{
    l_dir.setPath("logs/");
    if (!l_dir.exists())
        l_dir.mkpath(".");
}

void WriterFull::flush(const QString f_entry)
{
    l_logfile.setFileName("logs/server.log");

    if (l_logfile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        QTextStream file_stream(&l_logfile);
        file_stream << f_entry;
        l_logfile.close();
    }

    QFileInfo l_fileinfo("logs/server.log");
    if (QDateTime::currentDateTime().toSecsSinceEpoch() - l_fileinfo.birthTime().toSecsSinceEpoch() > 7776000) { // rename log file every 90 days
        QString newFileName = "logs/server-" + l_fileinfo.birthTime().toString("yyyy.MM.dd-HH.mm.ss") + "-" + QDateTime::currentDateTime().toString("yyyy.MM.dd-HH.mm.ss") + ".log";
        bool renamed = QFile::rename("logs/server.log", newFileName);

        if (renamed) {
            QFile file("logs/server.log");
            if (file.open(QIODevice::WriteOnly | QIODevice::Append)) {
                file.setFileTime(QDateTime::currentDateTime(), QFileDevice::FileBirthTime);
                file.close();
            }
        }
    }
}

void WriterFull::flush(const QString f_entry, const QString f_area_name)
{
    l_logfile.setFileName(QString("logs/%1.log").arg(f_area_name));

    if (l_logfile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        QTextStream file_stream(&l_logfile);
        file_stream << f_entry;
    }

    l_logfile.close();
};
