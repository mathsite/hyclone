#ifndef __SERVER_SYSTEMNOTIFICATION_H__
#define __SERVER_SYSTEMNOTIFICATION_H__

#include <algorithm>
#include <list>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>

#include "associateddata.h"
#include "BeDefs.h"
#include "server_notifications.h"

class KMessage;
class Port;

enum
{
    // team creation or deletion; object == -1; either one also triggers on
    // exec()
    B_WATCH_SYSTEM_TEAM_CREATION = 0x01,
    B_WATCH_SYSTEM_TEAM_DELETION = 0x02,

    // thread creation or deletion or property (name, priority) changes;
    // object == team ID or -1 for all teams
    B_WATCH_SYSTEM_THREAD_CREATION = 0x04,
    B_WATCH_SYSTEM_THREAD_DELETION = 0x08,
    B_WATCH_SYSTEM_THREAD_PROPERTIES = 0x10,

    B_WATCH_SYSTEM_ALL
        = B_WATCH_SYSTEM_TEAM_CREATION
        | B_WATCH_SYSTEM_TEAM_DELETION
        | B_WATCH_SYSTEM_THREAD_CREATION
        | B_WATCH_SYSTEM_THREAD_DELETION
        | B_WATCH_SYSTEM_THREAD_PROPERTIES
};

#define TEAM_MONITOR '_Tm_'
#define TEAM_ADDED   0x01
#define TEAM_REMOVED 0x02
#define TEAM_EXEC    0x04

// thread notifications
#define THREAD_MONITOR      '_tm_'
#define THREAD_ADDED        0x01
#define THREAD_REMOVED      0x02
#define THREAD_NAME_CHANGED 0x04

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmultichar"
#endif

enum
{
    // message what for the notification messages
    B_SYSTEM_OBJECT_UPDATE = 'SOUP',

    // "opcode" values
    B_TEAM_CREATED = 0,
    B_TEAM_DELETED = 1,
    B_TEAM_EXEC = 2,
    B_THREAD_CREATED = 3,
    B_THREAD_DELETED = 4,
    B_THREAD_NAME_CHANGED = 5
};

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

class SystemNotificationService : private NotificationListener
{
private:
    struct ListenerList;

    struct Listener : AssociatedData
    {
        std::list<std::shared_ptr<Listener>>::iterator listLink;
        ListenerList* list;
        port_id port;
        uint32 flags;
        int32 token;

        virtual void OwnerDeleted(AssociatedDataOwner* owner) override;
    };

    struct ListenerList
    {
        std::list<std::shared_ptr<Listener>> listeners;
        int32 object;
    };

    static const int32 kMaxMessagingTargetCount = 8;

    std::mutex _lock;
    std::unordered_map<int32, ListenerList> _teamListeners;

    virtual void EventOccurred(NotificationService& service, const KMessage* event);
    void _AddTargets(ListenerList* listenerList, uint32_t flags, messaging_target* targets,
        int32& targetCount, int32 object, uint32 opcode);
    void _SendMessage(messaging_target* targets, int32 targetCount, int32 object, uint32 opcode);
    std::shared_ptr<Listener> _FindListener(int32 object, port_id port, int32 token,
        ListenerList*& listeners);
    void _RemoveObsoleteListener(const std::shared_ptr<Listener>& listener);
    void _RemoveListener(const std::shared_ptr<Listener>& listener);
public:
    SystemNotificationService() = default;
    status_t Init();

    status_t StartListening(int32 object, uint32 flags, port_id port, int32 token);
    status_t StopListening(int32 object, uint32 flags, port_id port, int32 token);

    void Notify(const KMessage& event);
    std::unique_lock<std::mutex> Lock() { return std::unique_lock<std::mutex>(_lock); }
};

#endif // __SERVER_SYSTEMNOTIFICATION_H__