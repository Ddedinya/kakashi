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
#include "db_manager.h"
#include <QDir>

DBManager::DBManager() :
    DRIVER("QSQLITE")
{
    const QString db_filename = "logs/akashi.db";
    QFileInfo db_info(db_filename);
    if (!db_info.exists()) {
        QDir().mkdir("logs");
        qWarning().noquote() << tr("Database Info: Database not found. Attempting to create new database.");
    }
    else {
        // We should only check if a file is readable/writeable when it actually exists.
        if (!db_info.isReadable() || !db_info.isWritable())
            qCritical() << tr("Database Error: Missing permissions. Check if \"%1\" is writable.").arg(db_filename);
    }

    db = QSqlDatabase::addDatabase(DRIVER);
    db.setDatabaseName("logs/akashi.db");
    if (!db.open())
        qCritical() << "Database Error:" << db.lastError();

    QSqlQuery create_ban_table("CREATE TABLE IF NOT EXISTS bans ('ID' INTEGER, 'IPID' TEXT, 'HDID' TEXT, 'IP' TEXT, 'TIME' INTEGER, 'REASON' TEXT, 'DURATION' INTEGER, 'MODERATOR' TEXT, PRIMARY KEY('ID' AUTOINCREMENT))");
    QSqlQuery create_user_table("CREATE TABLE IF NOT EXISTS users ('ID' INTEGER, 'USERNAME' TEXT, 'SALT' TEXT, 'PASSWORD' TEXT, 'ACL' TEXT, PRIMARY KEY('ID' AUTOINCREMENT))");
    QSqlQuery create_ipidip_table("CREATE TABLE IF NOT EXISTS ipidip ('ID' INTEGER, 'IPID' TEXT, 'IP' TEXT, 'CREATED' TEXT, 'HWID' TEXT, PRIMARY KEY('ID' AUTOINCREMENT))");
    QSqlQuery create_automod_table("CREATE TABLE IF NOT EXISTS automod ('ID' INTEGER, 'IPID' TEXT, 'DATE' TEXT, 'ACTION' TEXT, 'HAZNUM' INTEGER, PRIMARY KEY('ID' AUTOINCREMENT))");
    QSqlQuery create_automodwarns_table("CREATE TABLE IF NOT EXISTS automodwarns ('ID' INTEGER, 'IPID' TEXT, 'DATE' TEXT, 'WARNS' INTEGER, PRIMARY KEY('ID' AUTOINCREMENT))");

    create_ban_table.exec();
    create_user_table.exec();
    create_ipidip_table.exec();
    create_automod_table.exec();
    create_automodwarns_table.exec();

    QSqlQuery query;
    query.prepare("SELECT HWID FROM ipidip");
    query.setForwardOnly(true);
    query.exec();
    if (!query.exec()) {
        query.clear();
        query.prepare("ALTER TABLE ipidip ADD HWID TEXT");
        if (!query.exec())
            qDebug() << "SQL Error:" << query.lastError().text();
    }

    db_version = checkVersion();
    if (db_version != DB_VERSION)
        updateDB(db_version);
}

QPair<bool, DBManager::BanInfo> DBManager::isIPBanned(QString ipid)
{
    QSqlQuery query;
    query.prepare("SELECT * FROM BANS WHERE IPID = ? ORDER BY TIME DESC");
    query.addBindValue(ipid);
    query.exec();
    BanInfo ban;
    if (query.first()) {
        ban.id = query.value(0).toInt();
        ban.ipid = query.value(1).toString();
        ban.hdid = query.value(2).toString();
        ban.ip = QHostAddress(query.value(3).toString());
        ban.time = static_cast<unsigned long>(query.value(4).toULongLong());
        ban.reason = query.value(5).toString();
        ban.duration = query.value(6).toLongLong();
        ban.moderator = query.value(7).toString();
        if (ban.duration == -2)
            return {true, ban};

        unsigned long current_time = QDateTime::currentDateTime().toSecsSinceEpoch();
        if (ban.time + ban.duration > current_time)
            return {true, ban};
        else
            return {false, ban};
    }
    else
        return {false, ban};
}

QPair<bool, DBManager::BanInfo> DBManager::isHDIDBanned(QString hdid)
{
    QSqlQuery query;
    query.prepare("SELECT * FROM BANS WHERE HDID = ? ORDER BY TIME DESC");
    query.addBindValue(hdid);
    query.exec();
    BanInfo ban;
    if (query.first()) {
        ban.id = query.value(0).toInt();
        ban.ipid = query.value(1).toString();
        ban.hdid = query.value(2).toString();
        ban.ip = QHostAddress(query.value(3).toString());
        ban.time = static_cast<unsigned long>(query.value(4).toULongLong());
        ban.reason = query.value(5).toString();
        ban.duration = query.value(6).toLongLong();
        ban.moderator = query.value(7).toString();
        if (ban.duration == -2)
            return {true, ban};

        unsigned long current_time = QDateTime::currentDateTime().toSecsSinceEpoch();
        if (ban.time + ban.duration > current_time)
            return {true, ban};
        else
            return {false, ban};
    }
    else
        return {false, ban};
}

int DBManager::getBanID(QString hdid)
{
    QSqlQuery query;
    query.prepare("SELECT ID FROM BANS WHERE HDID = ? ORDER BY TIME DESC");
    query.addBindValue(hdid);
    query.exec();
    if (query.first())
        return query.value(0).toInt();
    else
        return -1;
}

int DBManager::getBanID(QHostAddress ip)
{
    QSqlQuery query;
    query.prepare("SELECT ID FROM BANS WHERE IP = ? ORDER BY TIME DESC");
    query.addBindValue(ip.toString());
    query.exec();
    if (query.first())
        return query.value(0).toInt();
    else
        return -1;
}

QList<DBManager::BanInfo> DBManager::getRecentBans()
{
    QList<BanInfo> return_list;
    QSqlQuery query;
    query.prepare("SELECT * FROM BANS ORDER BY TIME DESC LIMIT 5");
    query.setForwardOnly(true);
    query.exec();
    while (query.next()) {
        BanInfo ban;
        ban.id = query.value(0).toInt();
        ban.ipid = query.value(1).toString();
        ban.hdid = query.value(2).toString();
        ban.ip = QHostAddress(query.value(3).toString());
        ban.time = static_cast<unsigned long>(query.value(4).toULongLong());
        ban.reason = query.value(5).toString();
        ban.duration = query.value(6).toLongLong();
        ban.moderator = query.value(7).toString();
        return_list.append(ban);
    }

    std::reverse(return_list.begin(), return_list.end());
    return return_list;
}

void DBManager::addBan(BanInfo ban)
{
    QSqlQuery query;
    QList<DBManager::idipinfo> l_ipidinfo = getIpidInfo(ban.ipid);
    query.prepare("INSERT INTO BANS(IPID, HDID, IP, TIME, REASON, DURATION, MODERATOR) VALUES(?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(ban.ipid);
    if (!ban.hdid.isEmpty())
        query.addBindValue(ban.hdid);
    else if (!l_ipidinfo.isEmpty())
        query.addBindValue(l_ipidinfo[0].hwid);
    else
        query.addBindValue("");

    query.addBindValue(ban.ip.toString());
    query.addBindValue(QString::number(ban.time));
    query.addBindValue(ban.reason);
    query.addBindValue(ban.duration);
    query.addBindValue(ban.moderator);

    if (!query.exec())
        qDebug() << "SQL Error:" << query.lastError().text();
}

bool DBManager::invalidateBan(int id)
{
    QSqlQuery ban_exists;
    ban_exists.prepare("SELECT DURATION FROM bans WHERE ID = ?");
    ban_exists.addBindValue(id);
    ban_exists.exec();
    if (!ban_exists.first())
        return false;

    QSqlQuery query;
    query.prepare("UPDATE bans SET DURATION = 0 WHERE ID = ?");
    query.addBindValue(id);
    query.exec();
    return true;
}

bool DBManager::createUser(QString f_username, QByteArray f_salt, QString f_password, QString f_acl)
{
    QSqlQuery username_exists;
    username_exists.prepare("SELECT ACL FROM users WHERE USERNAME = ?");
    username_exists.addBindValue(f_username);
    username_exists.exec();
    if (username_exists.first())
        return false;

    QSqlQuery query;
    QString salted_password = CryptoHelper::hash_password(f_salt, f_password);
    query.prepare("INSERT INTO users(USERNAME, SALT, PASSWORD, ACL) VALUES(?, ?, ?, ?)");
    query.addBindValue(f_username);
    query.addBindValue(f_salt.toHex());
    query.addBindValue(salted_password);
    query.addBindValue(f_acl);
    query.exec();
    return true;
}

bool DBManager::deleteUser(QString username)
{
    QSqlQuery username_exists;
    username_exists.prepare("SELECT ACL FROM users WHERE USERNAME = ?");
    username_exists.addBindValue(username);
    username_exists.exec();
    if (username_exists.first())
        return false;

    QSqlQuery query;
    query.prepare("DELETE FROM users WHERE USERNAME = ?");
    username_exists.addBindValue(username);
    username_exists.exec();
    return true;
}

QString DBManager::getACL(QString moderator_name)
{
    if (moderator_name == "")
        return 0;

    QSqlQuery query("SELECT ACL FROM users WHERE USERNAME = ?");
    query.addBindValue(moderator_name);
    query.exec();
    if (!query.first())
        return 0;

    return query.value(0).toString();
}

bool DBManager::authenticate(QString username, QString password)
{
    QSqlQuery query_salt("SELECT SALT FROM users WHERE USERNAME = ?");
    query_salt.addBindValue(username);
    query_salt.exec();
    if (!query_salt.first())
        return false;

    QString salt = query_salt.value(0).toString();
    QString salted_password = CryptoHelper::hash_password(QByteArray::fromHex(salt.toUtf8()), password);
    QSqlQuery query_pass("SELECT PASSWORD FROM users WHERE USERNAME = ?");
    query_pass.addBindValue(username);
    query_pass.exec();
    if (!query_pass.first())
        return false;

    QString stored_pass = query_pass.value(0).toString();
    // Update old-style hashes to new ones on the fly
    if (QByteArray::fromHex(salt.toUtf8()).length() < CryptoHelper::pbkdf2_salt_len && salted_password == stored_pass)
        updatePassword(username, password);

    return salted_password == stored_pass;
}

bool DBManager::updateACL(QString f_username, QString f_acl)
{
    QSqlQuery l_username_exists;
    l_username_exists.prepare("SELECT ACL FROM users WHERE USERNAME = ?");
    l_username_exists.addBindValue(f_username);
    l_username_exists.exec();
    if (!l_username_exists.first())
        return false;

    QSqlQuery l_update_acl;
    l_update_acl.prepare("UPDATE users SET ACL = ? WHERE USERNAME = ?");
    l_update_acl.addBindValue(f_acl);
    l_update_acl.addBindValue(f_username);
    l_update_acl.exec();
    return true;
}

QStringList DBManager::getUsers()
{
    QStringList users;
    QSqlQuery query("SELECT USERNAME FROM users ORDER BY ID");
    while (query.next())
        users.append(query.value(0).toString());

    return users;
}

QList<DBManager::BanInfo> DBManager::getBanInfo(QString lookup_type, QString id)
{
    QSqlQuery query;
    QList<BanInfo> invalid;
    if (lookup_type == "banid")
        query.prepare("SELECT * FROM BANS WHERE ID = ?");
    else if (lookup_type == "hdid")
        query.prepare("SELECT * FROM BANS WHERE HDID = ?");
    else if (lookup_type == "ipid")
        query.prepare("SELECT * FROM BANS WHERE IPID = ?");
    else {
        qCritical("Invalid ban lookup type!");
        return invalid;
    }

    query.addBindValue(id);
    query.setForwardOnly(true);
    query.exec();
    QList<BanInfo> return_list;
    while (query.next()) {
        BanInfo ban;
        ban.id = query.value(0).toInt();
        ban.ipid = query.value(1).toString();
        ban.hdid = query.value(2).toString();
        ban.ip = QHostAddress(query.value(3).toString());
        ban.time = static_cast<unsigned long>(query.value(4).toULongLong());
        ban.reason = query.value(5).toString();
        ban.duration = query.value(6).toLongLong();
        ban.moderator = query.value(7).toString();
        return_list.append(ban);
    }

    std::reverse(return_list.begin(), return_list.end());
    return return_list;
}

int DBManager::getHazNum(QString ipid)
{
    QSqlQuery query;
    query.prepare("SELECT * FROM AUTOMOD WHERE IPID = ?");
    query.addBindValue(ipid);
    query.setForwardOnly(true);
    query.exec();
    while (query.next()) {
        automod num;
        return query.value(4).toInt();
    }

    return 0;
}

long DBManager::getHazNumDate(QString ipid)
{
    QSqlQuery query;
    query.prepare("SELECT * FROM AUTOMOD WHERE IPID = ?");
    query.addBindValue(ipid);
    query.setForwardOnly(true);
    query.exec();
    while (query.next()) {
        automod num;
        return query.value(2).toLongLong();
    }

    return 0;
}

void DBManager::addHazNum(automod num)
{
    QSqlQuery query;
    query.prepare("INSERT INTO AUTOMOD(IPID, DATE, ACTION, HAZNUM) VALUES(?, ?, ?, ?)");
    query.addBindValue(num.ipid);
    query.addBindValue(QString::number(num.date));
    query.addBindValue(num.action);
    query.addBindValue(num.haznum);

    if (!query.exec())
        qDebug() << "SQL Error:" << query.lastError().text();
}

void DBManager::updateHazNum(QString ipid, long date)
{
    QSqlQuery query;
    query.prepare("UPDATE automod SET DATE = ? WHERE IPID = ?");
    query.addBindValue(QString::number(date));
    query.addBindValue(ipid);

    if (!query.exec())
        qDebug() << "SQL Error:" << query.lastError().text();
}

void DBManager::updateHazNum(QString ipid, int haznum)
{
    QSqlQuery query;
    query.prepare("UPDATE automod SET HAZNUM = ? WHERE IPID = ?");
    query.addBindValue(haznum);
    query.addBindValue(ipid);

    if (!query.exec())
        qDebug() << "SQL Error:" << query.lastError().text();
}

void DBManager::updateHazNum(QString ipid, QString action)
{
    QSqlQuery query;
    query.prepare("UPDATE automod SET ACTION = ? WHERE IPID = ?");
    query.addBindValue(action);
    query.addBindValue(ipid);

    if (!query.exec())
        qDebug() << "SQL Error:" << query.lastError().text();
}

bool DBManager::hazNumExist(QString ipid)
{
    QSqlQuery query;
    query.prepare("SELECT * FROM automod WHERE IPID = ?");
    query.addBindValue(ipid);
    query.setForwardOnly(true);
    query.exec();

    while (query.next()) {
        automodwarns warn;
        warn.ipid = query.value(1).toString();
        if (!warn.ipid.isEmpty())
            return true;
    }

    return false;
}

int DBManager::getWarnNum(QString ipid)
{
    QSqlQuery query;
    query.prepare("SELECT * FROM AUTOMODWARNS WHERE IPID = ?");
    query.addBindValue(ipid);
    query.setForwardOnly(true);
    query.exec();

    while (query.next()) {
        automodwarns warn;
        return query.value(3).toInt();
    }

    return 0;
}

long DBManager::getWarnDate(QString ipid)
{
    QSqlQuery query;
    query.prepare("SELECT * FROM AUTOMODWARNS WHERE IPID = ?");
    query.addBindValue(ipid);
    query.setForwardOnly(true);
    query.exec();

    while (query.next()) {
        automodwarns warn;
        return query.value(2).toLongLong();
    }

    return 0;
}

void DBManager::addWarn(automodwarns warn)
{
    QSqlQuery query;
    query.prepare("INSERT INTO AUTOMODWARNS(IPID, DATE, WARNS) VALUES(?, ?, ?)");
    query.addBindValue(warn.ipid);
    query.addBindValue(QString::number(warn.date));
    query.addBindValue(warn.warns);

    if (!query.exec())
        qDebug() << "SQL Error:" << query.lastError().text();
}

void DBManager::updateWarn(QString ipid, int warns)
{
    QSqlQuery query;
    query.prepare("UPDATE automodwarns SET WARNS = ? WHERE IPID = ?");
    query.addBindValue(warns);
    query.addBindValue(ipid);

    if (!query.exec())
        qDebug() << "SQL Error:" << query.lastError().text();
}

void DBManager::updateWarn(QString ipid, long date)
{
    QSqlQuery query;
    query.prepare("UPDATE automodwarns SET DATE = ? WHERE IPID = ?");
    query.addBindValue(QString::number(date));
    query.addBindValue(ipid);

    if (!query.exec())
        qDebug() << "SQL Error:" << query.lastError().text();
}

bool DBManager::warnExist(QString ipid)
{
    QSqlQuery query;
    query.prepare("SELECT * FROM automodwarns WHERE IPID = ?");
    query.addBindValue(ipid);
    query.setForwardOnly(true);
    query.exec();

    while (query.next()) {
        automodwarns warn;
        warn.ipid = query.value(1).toString();
        if (!warn.ipid.isEmpty())
            return true;
    }

    return false;
}

bool DBManager::updateBan(int ban_id, QString field, QVariant updated_info)
{
    QSqlQuery query;
    if (field == "reason") {
        query.prepare("UPDATE bans SET REASON = ? WHERE ID = ?");
        query.addBindValue(updated_info.toString());
    }
    else if (field == "duration") {
        query.prepare("UPDATE bans SET DURATION = ? WHERE ID = ?");
        query.addBindValue(updated_info.toLongLong());
    }

    query.addBindValue(ban_id);
    if (!query.exec()) {
        qDebug() << query.lastError();
        return false;
    }
    else
        return true;
}

bool DBManager::updatePassword(QString username, QString password)
{
    QByteArray salt = CryptoHelper::randbytes(16);
    QString salted_password = CryptoHelper::hash_password(salt, password);
    QSqlQuery query;
    query.prepare("UPDATE users SET PASSWORD = ?, SALT = ? WHERE USERNAME = ?");
    query.addBindValue(salted_password);
    query.addBindValue(salt.toHex());
    query.addBindValue(username);
    query.exec();

    return true;
}

int DBManager::checkVersion()
{
    QSqlQuery query;
    query.prepare("PRAGMA user_version");
    query.exec();

    if (query.first())
        return query.value(0).toInt();
    else
        return 0;
}

void DBManager::updateDB(int current_version)
{
    switch (current_version) {
    case 0:
        QSqlQuery("ALTER TABLE bans ADD COLUMN MODERATOR TEXT");
        Q_FALLTHROUGH();
    case 1:
        QSqlQuery("PRAGMA user_version = " + QString::number(1));
        Q_FALLTHROUGH();
    case 2:
        QSqlQuery("UPDATE users SET ACL = 'SUPER' WHERE USERNAME = 'root'");
        QSqlQuery("PRAGMA user_version = " + QString::number(DB_VERSION));
        break;
    }
}

bool DBManager::ipidExist(QString ipid)
{
    QSqlQuery query;

    query.prepare("SELECT * FROM IPIDIP WHERE IPID = ?");
    query.addBindValue(ipid);
    query.setForwardOnly(true);
    query.exec();

    while (query.next()) {
        idipinfo ipidip;
        ipidip.ipid = query.value(1).toString();
        ipidip.ip = query.value(2).toString();
        ipidip.date = query.value(3).toString();

        if (!ipidip.ip.isEmpty())
            return true;
    }

    return false;
}

void DBManager::ipidip(QString ipid, QString ip, QString date, QString hwid)
{
    if (ipidExist(ipid)) {
        QSqlQuery query;
        query.prepare("SELECT * FROM IPIDIP WHERE IPID = ?");
        query.addBindValue(ipid);
        query.setForwardOnly(true);
        query.exec();
        while (query.next()) {
            idipinfo ipidip;
            ipidip.hwid = query.value(4).toString();
            if (!hwid.isEmpty() && hwid != ipidip.hwid) {
                query.clear();
                query.prepare("UPDATE ipidip SET HWID = ? WHERE IPID = ?");
                query.addBindValue(hwid);
                query.addBindValue(ipid);
                if (!query.exec())
                    qDebug() << "SQL Error: " << query.lastError().text();
            }
        }

        return;
    }

    QSqlQuery query;
    query.prepare("INSERT INTO IPIDIP(IPID, IP, CREATED, HWID) VALUES(?, ?, ?, ?)");
    query.addBindValue(ipid);
    query.addBindValue(ip);
    query.addBindValue(date);
    query.addBindValue(hwid);

    if (!query.exec())
        qDebug() << "SQL Error: " << query.lastError().text();
}

QList<DBManager::idipinfo> DBManager::getIpidInfo(QString ipid)
{
    QSqlQuery query;
    query.prepare("SELECT * FROM IPIDIP WHERE IPID = ?");
    query.addBindValue(ipid);
    query.setForwardOnly(true);
    query.exec();

    QList<idipinfo> return_list;
    while (query.next()) {
        idipinfo ipidip;
        ipidip.ipid = query.value(1).toString();
        ipidip.ip = query.value(2).toString();
        ipidip.date = query.value(3).toString();
        ipidip.hwid = query.value(4).toString();
        return_list.append(ipidip);
    }

    std::reverse(return_list.begin(), return_list.end());
    return return_list;
}

DBManager::~DBManager() { db.close(); }
