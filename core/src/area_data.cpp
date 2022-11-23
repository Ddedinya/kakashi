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

#include <algorithm>

#include "include/area_data.h"
#include "include/config_manager.h"
#include "include/music_manager.h"
#include "include/packet/packet_factory.h"

AreaData::AreaData(QString p_name, int p_index, MusicManager *p_music_manager = nullptr) :
    m_index(p_index),
    m_music_manager(p_music_manager),
    m_playerCount(0),
    m_document("No document."),
    m_area_message("No area message set."),
    m_defHP(10),
    m_proHP(10),
    m_currentMusic("~stop.mp3"),
    m_statement(0),
    m_judgelog(),
    m_lastICMessage(),
    m_lastICMessageOwner(),
    m_send_area_message(false),
    m_can_send_wtce(true),
    m_can_use_shouts(true)
{
    QStringList name_split = p_name.split(":");
    name_split.removeFirst();
    m_hub = name_split[0].toInt();
    name_split.removeFirst();
    m_name = name_split.join(":");
    QSettings *areas_ini = ConfigManager::areaData();
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    areas_ini->setIniCodec("UTF-8");
#endif
    areas_ini->beginGroup(p_name);
    m_background = areas_ini->value("background", "gs4").toString();
    m_isProtected = areas_ini->value("protected_area", "false").toBool();
    m_iniswapAllowed = areas_ini->value("iniswap_allowed", "true").toBool();
    m_bgLocked = areas_ini->value("bg_locked", "false").toBool();
    m_eviMod = QVariant(areas_ini->value("evidence_mod", "FFA").toString().toUpper()).value<EvidenceMod>();
    m_status = QVariant(areas_ini->value("status", "IDLE").toString().toUpper()).value<AreaData::Status>();
    m_locked = QVariant(areas_ini->value("lock_status", "FREE").toString().toUpper()).value<AreaData::LockStatus>();
    m_blankpostingAllowed = areas_ini->value("blankposting_allowed", "true").toBool();
    m_area_message = areas_ini->value("area_message").toString();
    m_send_area_message = areas_ini->value("send_area_message_on_join", "false").toBool();
    m_can_send_wtce = areas_ini->value("wtce_enabled", "true").toBool();
    m_can_use_shouts = areas_ini->value("shouts_enabled", "true").toBool();
    m_forceImmediate = areas_ini->value("force_immediate", "false").toBool();
    m_toggleMusic = areas_ini->value("toggle_music", "true").toBool();
    m_shownameAllowed = areas_ini->value("shownames_allowed", "true").toBool();
    m_ignoreBgList = areas_ini->value("ignore_bglist", "false").toBool();
    m_chillMod = areas_ini->value("chillmod", "false").toBool();
    m_autoMod = areas_ini->value("automod", "false").toBool();
    m_floodguardactive = areas_ini->value("floodguard_active", "false").toBool();
    m_areapassword = areas_ini->value("password", "").toString();
    setEvidenceList(areas_ini->value("evidence", "").toStringList());
    m_can_change_status = areas_ini->value("change_status", "true").toBool();
    m_ooc_type = QVariant(areas_ini->value("ooc_type", "ALL").toString().toUpper()).value<AreaData::OocType>();
    areas_ini->endGroup();
    QTimer *timer1 = new QTimer();
    m_timers.append(timer1);
    QTimer *timer2 = new QTimer();
    m_timers.append(timer2);
    QTimer *timer3 = new QTimer();
    m_timers.append(timer3);
    QTimer *timer4 = new QTimer();
    m_timers.append(timer4);
    m_message_floodguard_timer = new QTimer(this);
    connect(m_message_floodguard_timer, &QTimer::timeout, this, &AreaData::allowMessage);
}

const QMap<QString, AreaData::Status> AreaData::map_statuses = {
    {"idle", AreaData::Status::IDLE},
    {"rp", AreaData::Status::RP},
    {"casing", AreaData::Status::CASING},
    {"lfp", AreaData::Status::LOOKING_FOR_PLAYERS},
    {"looking-for-players", AreaData::Status::LOOKING_FOR_PLAYERS},
    {"recess", AreaData::Status::RECESS},
    {"gaming", AreaData::Status::GAMING},
    {"erp", AreaData::Status::ERP},
    {"yablachki", AreaData::Status::YABLACHKI},
};

void AreaData::clientLeftArea(int f_charId, int f_userId)
{
    --m_playerCount;

    if (f_charId != -1) {
        m_charactersTaken.removeAll(f_charId);
    }
    m_joined_ids.removeAll(f_userId);
}

void AreaData::clientJoinedArea(int f_charId, int f_userId)
{
    ++m_playerCount;

    if (f_charId != -1) {
        m_charactersTaken.append(f_charId);
    }
    m_joined_ids.append(f_userId);
    emit userJoinedArea(m_index, f_userId);
}

QList<int> AreaData::owners() const
{
    return m_owners;
}
void AreaData::addOwner(int f_clientId)
{
    m_owners.append(f_clientId);
    m_invited.append(f_clientId);
}

bool AreaData::removeOwner(int f_clientId)
{
    const QList<int> lastowners = m_owners;

    m_owners.removeAll(f_clientId);
    m_invited.removeAll(f_clientId);

    if (!lastowners.isEmpty() && m_owners.isEmpty() && m_locked != AreaData::FREE) {
        m_locked = AreaData::FREE;
        return true;
    }

    return false;
}

bool AreaData::blankpostingAllowed() const
{
    return m_blankpostingAllowed;
}

void AreaData::toggleBlankposting()
{
    m_blankpostingAllowed = !m_blankpostingAllowed;
}

bool AreaData::isProtected() const
{
    return m_isProtected;
}

void AreaData::toggleIsProtected()
{
    m_isProtected = !m_isProtected;
}

AreaData::LockStatus AreaData::lockStatus() const
{
    return m_locked;
}

void AreaData::lock()
{
    m_locked = LockStatus::LOCKED;
}

void AreaData::unlock()
{
    m_locked = LockStatus::FREE;
}

void AreaData::spectatable()
{
    m_locked = LockStatus::SPECTATABLE;
}

bool AreaData::invite(int f_clientId)
{
    if (m_invited.contains(f_clientId)) {
        return false;
    }

    m_invited.append(f_clientId);
    return true;
}

bool AreaData::uninvite(int f_clientId)
{
    if (!m_invited.contains(f_clientId)) {
        return false;
    }

    m_invited.removeAll(f_clientId);
    return true;
}

int AreaData::playerCount() const
{
    return m_playerCount;
}

QList<QTimer *> AreaData::timers() const
{
    return m_timers;
}

QString AreaData::name() const
{
    return m_name;
}

int AreaData::index() const
{
    return m_index;
}

QList<int> AreaData::charactersTaken() const
{
    return m_charactersTaken;
}

bool AreaData::changeCharacter(int f_from, int f_to, bool taketaked)
{
    if (m_charactersTaken.contains(f_to) && taketaked == false) {
        return false;
    }

    if (f_to != -1) {
        if (f_from != -1) {
            m_charactersTaken.removeAll(f_from);
        }

        m_charactersTaken.append(f_to);
        return true;
    }

    if (f_to == -1 && f_from != -1) {
        m_charactersTaken.removeAll(f_from);
    }

    return false;
}

QList<AreaData::Evidence> AreaData::evidence() const
{
    return m_evidence;
}

void AreaData::swapEvidence(int f_eviId1, int f_eviId2)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 13, 0)
    // swapItemsAt does not exist in Qt older than 5.13
    m_evidence.swap(f_eviId1, f_eviId2);
#else
    m_evidence.swapItemsAt(f_eviId1, f_eviId2);
#endif
}

void AreaData::appendEvidence(const AreaData::Evidence &f_evi_r)
{
    m_evidence.append(f_evi_r);
}

void AreaData::deleteEvidence(int f_eviId)
{
    m_evidence.removeAt(f_eviId);
}

void AreaData::replaceEvidence(int f_eviId, const AreaData::Evidence &f_newEvi_r)
{
    m_evidence.replace(f_eviId, f_newEvi_r);
}

AreaData::Status AreaData::status() const
{
    return m_status;
}

bool AreaData::changeStatus(const QString &f_newStatus_r)
{
    if (AreaData::map_statuses.contains(f_newStatus_r)) {
        m_status = AreaData::map_statuses[f_newStatus_r];
        return true;
    }

    return false;
}

QList<int> AreaData::invited() const
{
    return m_invited;
}

bool AreaData::isMusicAllowed() const
{
    return m_toggleMusic;
}

bool AreaData::isMessageAllowed() const
{
    return m_can_send_ic_messages;
}

bool AreaData::isWtceAllowed() const
{
    return m_can_send_wtce;
}

bool AreaData::isShoutAllowed() const
{
    return m_can_use_shouts;
}

void AreaData::startMessageFloodguard(int f_duration)
{
    m_can_send_ic_messages = false;
    m_message_floodguard_timer->start(f_duration);
}

void AreaData::toggleMusic()
{
    m_toggleMusic = !m_toggleMusic;
}

void AreaData::setEviMod(const EvidenceMod &f_eviMod_r)
{
    m_eviMod = f_eviMod_r;
}

void AreaData::setTestimonyRecording(const TestimonyRecording &f_testimonyRecording_r)
{
    m_testimonyRecording = f_testimonyRecording_r;
}

void AreaData::restartTestimony()
{
    m_testimonyRecording = TestimonyRecording::PLAYBACK;
    m_statement = 0;
}

void AreaData::clearTestimony()
{
    m_testimonyRecording = AreaData::TestimonyRecording::STOPPED;
    m_statement = -1;
    m_testimony.clear();
}

bool AreaData::forceImmediate() const
{
    return m_forceImmediate;
}

void AreaData::toggleImmediate()
{
    m_forceImmediate = !m_forceImmediate;
}

const QStringList &AreaData::lastICMessage() const
{
    return m_lastICMessage;
}

const QString &AreaData::lastICMessageOwner() const
{
    return m_lastICMessageOwner;
}

void AreaData::updateLastICMessageOwner(const QString &f_lastMessageOwner_r)
{
    m_lastICMessageOwner = f_lastMessageOwner_r;
}

void AreaData::updateLastICMessage(const QStringList &f_lastMessage_r)
{
    m_lastICMessage = f_lastMessage_r;
}

QStringList AreaData::judgelog() const
{
    return m_judgelog;
}

void AreaData::appendJudgelog(const QString &f_newLog_r)
{
    if (m_judgelog.size() == 10) {
        m_judgelog.removeFirst();
    }

    m_judgelog.append(f_newLog_r);
}

int AreaData::statement() const
{
    return m_statement;
}

void AreaData::recordStatement(const QStringList &f_newStatement_r)
{
    ++m_statement;
    m_testimony.append(f_newStatement_r);
}

void AreaData::addStatement(int f_position, const QStringList &f_newStatement_r)
{
    m_testimony.insert(f_position, f_newStatement_r);
}

void AreaData::replaceStatement(int f_position, const QStringList &f_newStatement_r)
{
    m_testimony.replace(f_position, f_newStatement_r);
}

void AreaData::removeStatement(int f_position)
{
    m_testimony.remove(f_position);
    --m_statement;
}

QPair<QStringList, AreaData::TestimonyProgress> AreaData::jumpToStatement(int f_position)
{
    m_statement = f_position;

    if (m_statement > m_testimony.size() - 1) {
        m_statement = 1;
        return {m_testimony.at(m_statement), TestimonyProgress::LOOPED};
    }
    if (m_statement <= 1) {
        m_statement = 1;
        return {m_testimony.at(m_statement), TestimonyProgress::STAYED_AT_FIRST};
    }
    else {
        return {m_testimony.at(m_statement), TestimonyProgress::OK};
    }
}

const QVector<QStringList> &AreaData::testimony() const
{
    return m_testimony;
}

AreaData::TestimonyRecording AreaData::testimonyRecording() const
{
    return m_testimonyRecording;
}

AreaData::EvidenceMod AreaData::eviMod() const
{
    return m_eviMod;
}

bool AreaData::addNotecard(const QString &f_owner_r, const QString &f_notecard_r)
{
    m_notecards[f_owner_r] = f_notecard_r;

    if (f_notecard_r.isNull()) {
        m_notecards.remove(f_owner_r);
        return false;
    }

    return true;
}

QStringList AreaData::getNotecards()
{
    QMapIterator<QString, QString> l_noteIter(m_notecards);
    QStringList l_notecards;

    while (l_noteIter.hasNext()) {
        l_noteIter.next();
        l_notecards << l_noteIter.key() << ": " << l_noteIter.value() << "\n";
    }

    m_notecards.clear();

    return l_notecards;
}

QString AreaData::musicPlayerBy() const
{
    return m_musicPlayedBy;
}

void AreaData::setMusicPlayedBy(const QString &f_music_player)
{
    m_musicPlayedBy = f_music_player;
}

void AreaData::changeMusic(const QString &f_source_r, const QString &f_newSong_r)
{
    m_currentMusic = f_newSong_r;
    m_musicPlayedBy = f_source_r;
}

QString AreaData::currentMusic() const
{
    return m_currentMusic;
}

void AreaData::setCurrentMusic(QString f_current_song)
{
    m_currentMusic = f_current_song;
}

int AreaData::proHP() const
{
    return m_proHP;
}

void AreaData::changeHP(AreaData::Side f_side, int f_newHP)
{
    if (f_side == Side::DEFENCE) {
        m_defHP = std::min(std::max(0, f_newHP), 10);
    }
    else if (f_side == Side::PROSECUTOR) {
        m_proHP = std::min(std::max(0, f_newHP), 10);
    }
}

int AreaData::defHP() const
{
    return m_defHP;
}

QString AreaData::document() const
{
    return m_document;
}

void AreaData::changeDoc(const QString &f_newDoc_r)
{
    m_document = f_newDoc_r;
}

QString AreaData::areaMessage() const
{
    return m_area_message.isEmpty() ? "No area message set." : m_area_message;
}

bool AreaData::sendAreaMessageOnJoin() const
{
    return m_send_area_message;
}

void AreaData::changeAreaMessage(const QString &f_newMessage_r)
{
    m_area_message = f_newMessage_r;
}

void AreaData::clearAreaMessage()
{
    changeAreaMessage(QString{});
}

bool AreaData::bgLocked() const
{
    return m_bgLocked;
}

void AreaData::toggleBgLock()
{
    m_bgLocked = !m_bgLocked;
}

bool AreaData::iniswapAllowed() const
{
    return m_iniswapAllowed;
}

void AreaData::toggleIniswap()
{
    m_iniswapAllowed = !m_iniswapAllowed;
}

bool AreaData::shownameAllowed() const
{
    return m_shownameAllowed;
}

QString AreaData::background() const
{
    return m_background;
}

void AreaData::setBackground(const QString f_background)
{
    m_background = f_background;
}

bool AreaData::ignoreBgList()
{
    return m_ignoreBgList;
}

void AreaData::toggleIgnoreBgList()
{
    m_ignoreBgList = !m_ignoreBgList;
}

void AreaData::toggleAreaMessageJoin()
{
    m_send_area_message = !m_send_area_message;
}

void AreaData::toggleWtceAllowed()
{
    m_can_send_wtce = !m_can_send_wtce;
}

void AreaData::toggleShoutAllowed()
{
    m_can_use_shouts = !m_can_use_shouts;
}

bool AreaData::floodguardActive()
{
    return m_floodguardactive;
}

void AreaData::toggleFloodguardActive()
{
    m_floodguardactive = !m_floodguardactive;
}

bool AreaData::chillMod()
{
    return m_chillMod;
}

void AreaData::toggleChillMod()
{
    m_chillMod = !m_chillMod;
}

bool AreaData::autoMod()
{
    return m_autoMod;
}

void AreaData::toggleAutoMod()
{
    m_autoMod = !m_autoMod;
}

QString AreaData::areaPassword()
{
    return m_areapassword;
}

void AreaData::setAreaPassword(QString f_password)
{
    m_areapassword = f_password;
}

bool AreaData::allowChangeStatus()
{
    return m_can_change_status;
}

void AreaData::toggleChangeStatus()
{
    m_can_change_status = !m_can_change_status;
}

void AreaData::setEvidenceList(QStringList f_evi_list)
{
    for (int i = 0; i < f_evi_list.size(); i++) {
        QRegularExpression l_regex(QString::number(i) + "(?:(?:(?<name>.*?)name)*(?:(?<desc>.*?)desc)*(?:(?<image>.*?)image)*)");
        QRegularExpressionMatch l_match = l_regex.match(f_evi_list[i]);
        QString l_evi_name, l_evi_desc, l_evi_image;

        if (l_match.hasMatch()) {
            l_evi_name = l_match.captured("name");
            l_evi_desc = l_match.captured("desc");
            l_evi_image = l_match.captured("image");
            AreaData::Evidence l_evi = {l_evi_name, l_evi_desc, l_evi_image};

            appendEvidence(l_evi);
        }
        else
            return;
    }
}

QVector<int> AreaData::joinedIDs() const
{
    return m_joined_ids;
}

void AreaData::allowMessage()
{
    m_can_send_ic_messages = true;
}

bool AreaData::isVoteStarted() const
{
    return m_vote_started;
}

void AreaData::toggleVote()
{
    m_vote_started = !m_vote_started;
}

AreaData::OocType AreaData::oocType() const
{
    return m_ooc_type;
}

void AreaData::setOocType(const OocType &f_oocType_r)
{
    m_ooc_type = f_oocType_r;
}

int AreaData::getHub()
{
    return m_hub;
}

void AreaData::setHub(int f_index)
{
    m_hub = f_index;
}
