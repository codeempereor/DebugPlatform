#include "core/communicationfactory.h"
#include "core/serialportcommunication.h"
#include "core/tcpcommunication.h"
#include "core/udpcommunication.h"
#include "core/mqttcommunication.h"

#include <QDebug>

// ============================================================================
// 静态成员定义
// ============================================================================

QHash<CommunicationType, CommunicationFactory::CreatorFunc> &
CommunicationFactory::creators()
{
    static QHash<CommunicationType, CreatorFunc> creatorsMap;
    // 【先设标志=已初始化 → 再调 initDefaultCreators】，避免递归死锁：
    //   initDefaultCreators → registerCreator → creators() → 此时 initialized
    //   已为 true，不会再进入此分支，也就不会重新调 initDefaultCreators 爆栈。
    if (!initialized()) {
        initialized() = true;
        initDefaultCreators();
    }
    return creatorsMap;
}

bool &CommunicationFactory::initialized()
{
    static bool isInitialized = false;
    return isInitialized;
}

void CommunicationFactory::initDefaultCreators()
{
    // 【入口再一层静态防重入】双保险：未来即使外部乱调 initDefaultCreators
    // 也不会重复注册三种通信类型。
    static bool s_inited = false;
    if (s_inited) return;
    s_inited = true;

    // 注册默认三种通信类型
    registerCreator(CommunicationType::Serial, []() -> ICommunication* {
        return new SerialPortCommunication();
    });

    registerCreator(CommunicationType::Tcp, []() -> ICommunication* {
        return new TcpCommunication();
    });

    registerCreator(CommunicationType::Udp, []() -> ICommunication* {
        return new UdpCommunication();
    });

    registerCreator(CommunicationType::Mqtt, []() -> ICommunication* {
        return new MqttCommunication();
    });
}

// ============================================================================
// 接口实现
// ============================================================================

ICommunication *CommunicationFactory::create(CommunicationType type,
                                              const CommunicationConfig &config)
{
    auto &creatorsMap = creators();

    auto it = creatorsMap.find(type);
    if (it == creatorsMap.end()) {
        qWarning() << "CommunicationFactory: Unknown communication type:"
                   << static_cast<int>(type);
        return nullptr;
    }

    ICommunication *comm = it.value()();
    if (comm) {
        comm->setConfig(config);
    }
    return comm;
}

bool CommunicationFactory::registerCreator(CommunicationType type,
                                             CreatorFunc creator)
{
    auto &creatorsMap = creators();

    if (creatorsMap.contains(type)) {
        qWarning() << "CommunicationFactory: Type already registered:"
                   << static_cast<int>(type);
        return false;
    }

    creatorsMap.insert(type, creator);
    return true;
}

bool CommunicationFactory::unregisterCreator(CommunicationType type)
{
    auto &creatorsMap = creators();

    if (!creatorsMap.contains(type)) {
        return false;
    }

    creatorsMap.remove(type);
    return true;
}

bool CommunicationFactory::isRegistered(CommunicationType type)
{
    auto &creatorsMap = creators();
    return creatorsMap.contains(type);
}

QList<CommunicationType> CommunicationFactory::registeredTypes()
{
    auto &creatorsMap = creators();
    return creatorsMap.keys();
}