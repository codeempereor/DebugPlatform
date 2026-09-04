#ifndef DEBUGPLATFORM_COMMUNICATIONFACTORY_H
#define DEBUGPLATFORM_COMMUNICATIONFACTORY_H

/**
 * @file communicationfactory.h
 * @brief 通信工厂类
 *
 * 提供通信对象的统一创建接口，支持运行时注册新的通信类型。
 * 使用 QHash 存储创建函数，实现开放-封闭原则。
 */

#include "core/icommunication.h"
#include "config.h"

#include <QHash>
#include <QString>
#include <functional>

/**
 * @class CommunicationFactory
 * @brief 通信工厂类
 *
 * 提供静态方法用于创建各种通信对象。支持通过注册机制
 * 扩展新的通信类型，便于后续添加更多通信协议。
 *
 * 使用示例：
 * @code
 *   ICommunication* serial = CommunicationFactory::create(
 *       CommunicationType::Serial, config);
 *   serial->open();
 * @endcode
 */
class CommunicationFactory
{
public:
    /**
     * @brief 通信对象创建函数类型
     */
    using CreatorFunc = std::function<ICommunication*()>;

    /**
     * @brief 创建通信对象
     * @param type 通信类型
     * @param config 通信配置
     * @return 指向创建的通信对象的指针，失败返回 nullptr
     * @note 调用者负责释放返回的对象
     */
    static ICommunication *create(CommunicationType type,
                                  const CommunicationConfig &config);

    /**
     * @brief 注册自定义通信类型的创建函数
     * @param type 通信类型
     * @param creator 创建函数
     * @return true 注册成功，false 类型已存在
     *
     * 允许在运行时注册新的通信类型，扩展工厂的功能。
     * 如果类型已注册，则注册失败。
     */
    static bool registerCreator(CommunicationType type, CreatorFunc creator);

    /**
     * @brief 注销通信类型
     * @param type 要注销的通信类型
     * @return true 注销成功，false 类型不存在
     */
    static bool unregisterCreator(CommunicationType type);

    /**
     * @brief 检查通信类型是否已注册
     * @param type 通信类型
     * @return true 已注册，false 未注册
     */
    static bool isRegistered(CommunicationType type);

    /**
     * @brief 获取所有已注册的通信类型
     * @return 通信类型列表
     */
    static QList<CommunicationType> registeredTypes();

private:
    /**
     * @brief 获取创建函数字典（懒初始化）
     * @return 创建函数字典的引用
     */
    static QHash<CommunicationType, CreatorFunc> &creators();

    /**
     * @brief 初始化默认通信类型
     *        在首次使用时自动注册 Serial、Tcp、Udp 三种类型
     */
    static void initDefaultCreators();

    /**
     * @brief 是否已初始化
     */
    static bool &initialized();
};

#endif // DEBUGPLATFORM_COMMUNICATIONFACTORY_H