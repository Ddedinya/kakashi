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
#ifndef U_LOGGER_H
#define U_LOGGER_H

#include "logger/writer_full.h"
#include "logger/writer_modcall.h"
#include <QDateTime>
#include <QMap>
#include <QObject>
#include <QQueue>

/**
 * @brief The Universal Logger class to provide a common place to handle, store and write logs to file.
 */
class ULogger : public QObject
{
    Q_OBJECT

  public:
    /**
     * @brief Constructor for the universal logger. Determines which writer is initially created.
     * @param Pointer to the Server.
     */
    ULogger(QObject *parent = nullptr);

    /**
     * @brief Deconstructor of the universal logger. Deletes its writer before deconstruction.
     */
    virtual ~ULogger();

    /**
     * @brief Returns the buffer of a respective area. Primarily used by the Discord Webhook.
     * @param Name of the area which buffer is requested.
     */
    QQueue<QString> buffer(const QString &f_areaName);

  public slots:

    /**
     * @brief Adds an IC log entry to the area buffer and writes it to the respective log format.
     */
    void logIC(const QString &f_char_name, const QString &f_ooc_name, const QString &f_ipid,
               const QString &f_area_name, const QString &f_message, const QString &f_uid,
               const QString &f_hwid, const QString &f_hub);

    /**
     * @brief Adds an OOC log entry to the area buffer and writes it to the respective log format.
     */
    void logOOC(const QString &f_char_Name, const QString &f_ooc_name, const QString &f_ipid,
                const QString &f_area_name, const QString &f_message, const QString &f_uid,
                const QString &f_hwid, const QString &f_hub);

    /**
     * @brief Adds an login attempt to the area buffer and writes it to the respective log format.
     */
    void logLogin(const QString &f_char_name, const QString &f_ooc_name, const QString &f_moderator_name,
                  const QString &f_ipid, const QString &f_area_name, const bool &f_success, const QString &f_uid,
                  const QString &f_hwid);

    /**
     * @brief Adds a command usage to the area buffer and writes it to the respective log format.
     */
    void logCMD(const QString &f_char_name, const QString &f_ipid, const QString &f_ooc_name, const QString &f_command,
                const QString &f_args, const QString &f_area_name, const QString &f_uid, const QString &f_hwid, const QString &f_hub);

    /**
     * @brief Adds a player kick to the area buffer and writes it to the respective log format.
     */
    void logKick(const QString &f_moderator, const QString &f_target_ipid, const QString &f_uid, const QString &f_hwid);

    /**
     * @brief Adds a player ban to the area buffer and writes it to the respective log format.
     */
    void logBan(const QString &f_moderator, const QString &f_target_ipid, const QString &f_duration, const QString &f_uid,
                const QString &f_hwid);

    /**
     * @brief Adds a modcall event to the area buffer, also triggers modcall writing.
     */
    void logModcall(const QString &f_char_name, const QString &f_ipid, const QString &f_ooc_name, const QString &f_area_name,
                    const QString &f_uid, const QString &f_hwid, const QString &f_hub);

    /**
     * @brief Logs any connection attempt to the server, whether sucessful or not.
     */
    void logConnectionAttempt(const QString &f_ipid, const QString &f_hwid);

    /**
     * @brief Logs any client disconnects.
     */
    void logDisconnect(const QString &f_char_name, const QString &f_ipid, const QString &f_ooc_name, const QString &f_area_name,
                       const QString &f_uid, const QString &f_hwid, const QString &f_hub);

    /**
     * @brief Logs any music change in areas.
     */
    void logMusic(const QString &f_char_Name, const QString &f_ooc_name, const QString &f_ipid,
                  const QString &f_area_name, const QString &f_music, const QString &f_uid,
                  const QString &f_hwid, const QString &f_hub);

    /**
     * @brief Logs any client's character change.
     */
    void logChangeChar(const QString &f_char_Name, const QString &f_ooc_name, const QString &f_ipid,
                       const QString &f_area_name, const QString &f_changechar, const QString &f_uid,
                       const QString &f_hwid, const QString &f_hub);

    /**
     * @brief Logs any client's area change.
     */
    void logChangeArea(const QString &f_char_Name, const QString &f_ooc_name, const QString &f_ipid,
                       const QString &f_area_name, const QString &f_changearea, const QString &f_uid,
                       const QString &f_hwid, const QString &f_hub);

    /**
     * @brief Loads template strings for the logger.
     */
    void loadLogtext();

  private:
    /**
     * @brief Updates the area buffer with a new entry, moving old ones if the buffer exceesed the maximum size.
     * @param Name of the area which buffer is modified.
     * @param Formatted QString to be added into the buffer.
     */
    void updateAreaBuffer(const QString &f_areaName, const QString &f_log_entry);

    /**
     * @brief QMap of all available area buffers.
     *
     * @details This QMap uses the area name as the index key to access its respective buffer.
     */
    QMap<QString, QQueue<QString>> m_bufferMap;

    /**
     * @brief Pointer to modcall writer. Handles QQueue delogging into area specific file.
     */
    WriterModcall *writerModcall;

    /**
     * @brief Pointer to full writer. Handles single messages in one file.
     */
    WriterFull *writerFull;

    /**
     * @brief Table that contains template strings for text-based logger format.
     * @details To keep ConfigManager cleaner the logstrings are loaded from an inifile by name.
     *          This has the problem of lacking defaults that work for all when the file is missing.
     *          This QMap contains all default values and overwrites them on logger construction.
     */
    QHash<QString, QString> m_logtext{
        {"ic", "[%1][%5][%9][IC][%2(%3)][%4][%7][%8]: %6"},
        {"ooc", "[%1][%5][%9][OOC][%2(%3)][%4][%7][%8]: %6"},
        {"login", "[%1][LOGIN][%2][%3][%4(%5)][%6][%7]"},
        {"cmdlogin", "[%1][%2][%8][LOGIN][%5][%3(%4)][%6][%7]"},
        {"cmdrootpass", "[%1][%2][%8][ROOTPASS][%5][%3(%4)][%6][%7]"},
        {"cmdadduser", "[%1][%2][%9][USERADD][%6][%3(%4)][%7][%8]: %5"},
        {"cmd", "[%1][%2][%5][%7][%3(%4)][%8][%9]: %6"},
        {"kick", "[%1][%2][%4][%5][KICK][%3]"},
        {"ban", "[%1][%2][%5][%6][BAN][%3][%4]"},
        {"modcall", "[%1][%2][%8][MODCALL][%5][%3(%4)][%6][%7]"},
        {"connect", "[%1][CONNECT][%2][%3]"},
        {"disconnect", "[%1][%2][%8][DISCONNECT][%5][%3(%4)][%6][%7]"},
        {"music", "[%1][%5][%9][CHANGEMUSIC][%4][%2(%3)][%7][%8]: %6"},
        {"changechar", "[%1][%5][%9][CHANGECHAR][%4][%2(%3)][%7][%8]: %6"},
        {"changearea", "[%1][%5][%9][CHANGEAREA][%2(%3)][%4][%7][%8]: %6"},
    };
};

#endif // U_LOGGER_H
