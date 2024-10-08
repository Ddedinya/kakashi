#include "hub_data.h"
#include "config_manager.h"

HubData::HubData(QString p_name, int p_index) :
    m_hub_index(p_index)
{
    QStringList name_split = p_name.split(":");
    name_split.removeFirst();
    m_hub_name = name_split.join(";");
    QSettings *hubs_ini = ConfigManager::hubsData();
    hubs_ini->beginGroup(p_name);
    m_hub_protected = hubs_ini->value("protected_hub", "false").toBool();
    m_hub_player_count_hide = hubs_ini->value("hide_playercount", "false").toBool();
    m_hub_locked = QVariant(hubs_ini->value("lock_status", "FREE").toString().toUpper()).value<HubData::HubLockStatus>();
    hubs_ini->endGroup();
}

QList<int> HubData::hubOwners() const { return m_hub_owners; }

void HubData::addHubOwner(int f_id)
{
    m_hub_owners.append(f_id);
    m_hub_invited.append(f_id);
}

bool HubData::removeHubOwner(int f_id)
{
    m_hub_owners.removeAll(f_id);
    m_hub_invited.removeAll(f_id);

    if (m_hub_owners.isEmpty() && m_hub_locked != HubData::FREE) {
        m_hub_locked = HubData::FREE;
        return true;
    }

    return false;
}

bool HubData::hubProtected() const { return m_hub_protected; }

void HubData::toggleHubProtected() { m_hub_protected = !m_hub_protected; }

void HubData::removeClient() { --m_hub_player_count; }

void HubData::addClient() { ++m_hub_player_count; }

int HubData::getHubPlayerCount() const { return m_hub_player_count; }

bool HubData::getHidePlayerCount() const { return m_hub_player_count_hide; }

void HubData::toggleHidePlayerCount() { m_hub_player_count_hide = !m_hub_player_count_hide; }

HubData::HubLockStatus HubData::hubLockStatus() const { return m_hub_locked; }

void HubData::hubLock() { m_hub_locked = HubLockStatus::LOCKED; }

void HubData::hubUnlock() { m_hub_locked = HubLockStatus::FREE; }

void HubData::hubSpectatable() { m_hub_locked = HubLockStatus::SPECTATABLE; }

QList<int> HubData::hubInvited() const { return m_hub_invited; }

bool HubData::hubInvite(int f_id)
{
    if (m_hub_invited.contains(f_id))
        return false;

    m_hub_invited.append(f_id);
    return true;
}

bool HubData::hubUninvite(int f_id)
{
    if (!m_hub_invited.contains(f_id))
        return false;

    m_hub_invited.removeAll(f_id);
    return true;
}
