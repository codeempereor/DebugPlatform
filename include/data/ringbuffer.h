#ifndef DEBUGPLATFORM_RINGBUFFER_H
#define DEBUGPLATFORM_RINGBUFFER_H

/**
 * @file ringbuffer.h
 * @brief 线程安全的环形缓冲区模板类
 *
 * 基于 std::vector 实现的固定容量环形缓冲区，支持并发读写。
 * 当缓冲区满时，可根据配置选择覆盖最旧数据或拒绝写入。
 * 使用 std::mutex 保护并发访问。
 */

#include <vector>
#include <mutex>
#include <cstddef>
#include <cstdint>
#include "core/config.h"

/**
 * @class RingBuffer
 * @brief 环形缓冲区模板类
 * @tparam T 缓冲区元素类型
 *
 * 典型的生产者-消费者场景下使用。支持多线程并发访问，
 * 通过互斥锁保证数据一致性。
 */
template <typename T>
class RingBuffer
{
public:
    /**
     * @brief 构造函数
     * @param capacity 缓冲区总容量
     * @param overwrite 当缓冲区满时是否覆盖最旧数据，默认为 true
     */
    explicit RingBuffer(size_t capacity, bool overwrite = true)
        : m_buffer(capacity)
        , m_capacity(capacity)
        , m_overwrite(overwrite)
        , m_head(0)
        , m_tail(0)
        , m_count(0)
    {
    }

    /**
     * @brief 写入一个元素
     * @param value 要写入的值
     * @return true 写入成功；false 当缓冲区已满且 overwrite 为 false 时
     */
    bool write(const T &value)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_count == m_capacity) {
            if (!m_overwrite) {
                return false;
            }
            // 覆盖最旧数据：移动 tail 到下一个位置
            m_tail = (m_tail + 1) % m_capacity;
        }

        m_buffer[m_head] = value;
        m_head = (m_head + 1) % m_capacity;

        if (m_count < m_capacity) {
            ++m_count;
        } else {
            // 覆盖模式：count 保持为 capacity
            m_count = m_capacity;
        }
        return true;
    }

    /**
     * @brief 读取一个元素（从队尾读取）
     * @param value 读取到的值将写入此引用
     * @return true 读取成功；false 当缓冲区为空时
     */
    bool read(T &value)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_count == 0) {
            return false;
        }

        value = m_buffer[m_tail];
        m_tail = (m_tail + 1) % m_capacity;
        --m_count;
        return true;
    }

    /**
     * @brief 可读元素数量
     * @return 当前可读取的元素数量
     */
    size_t availableRead() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_count;
    }

    /**
     * @brief 可写元素数量
     * @return 当前可写入的元素数量
     */
    size_t availableWrite() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_capacity - m_count;
    }

    /**
     * @brief 清空缓冲区
     */
    void clear()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_head = 0;
        m_tail = 0;
        m_count = 0;
    }

    /**
     * @brief 获取总容量
     * @return 缓冲区总容量
     */
    size_t capacity() const
    {
        return m_capacity;
    }

    /**
     * @brief 判断缓冲区是否为空
     * @return true 表示为空
     */
    bool isEmpty() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_count == 0;
    }

    /**
     * @brief 判断缓冲区是否已满
     * @return true 表示已满
     */
    bool isFull() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_count == m_capacity;
    }

    DISABLE_COPY(RingBuffer)

private:
    mutable std::mutex m_mutex;     ///< 互斥锁
    std::vector<T>      m_buffer;   ///< 底层存储
    size_t              m_capacity;  ///< 总容量
    bool                m_overwrite;///< 满时是否覆盖最旧数据
    size_t              m_head;     ///< 写指针（下一个写入位置）
    size_t              m_tail;     ///< 读指针（下一个读取位置）
    size_t              m_count;    ///< 当前元素数量
};

#endif // DEBUGPLATFORM_RINGBUFFER_H
