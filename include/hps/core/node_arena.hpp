#pragma once

#include "hps/core/node.hpp"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <new>
#include <vector>

namespace hps {

/**
 * @brief DOM 节点的单调 arena 分配器。
 *
 * 所有节点对象由 arena 拥有，节点之间用裸指针互相引用，生命周期 = arena（Document）
 * 生命周期。分配为块内 bump（摊销零 malloc）；析构时按逆序调用每个节点的虚析构函数，
 * 再释放内存块。不支持单独释放某个节点（节点随 Document 整体回收）。
 */
class NodeArena {
  public:
    NodeArena() = default;

    NodeArena(const NodeArena&)            = delete;
    NodeArena& operator=(const NodeArena&) = delete;
    NodeArena(NodeArena&&)                 = default;
    NodeArena& operator=(NodeArena&&)      = default;

    ~NodeArena() {
        // 逆序析构（后建的先析构）；虚析构分发到派生类，释放各自的字符串等成员。
        for (auto it = m_nodes.rbegin(); it != m_nodes.rend(); ++it) {
            (*it)->~Node();
        }
    }

    /**
     * @brief 在 arena 中构造一个节点并返回其指针（所有权归 arena）。
     */
    template <class T, class... Args>
    T* make(Args&&... args) {
        void* memory = allocate(sizeof(T), alignof(T));
        T*    node   = ::new (memory) T(std::forward<Args>(args)...);
        m_nodes.push_back(node);
        return node;
    }

  private:
    static constexpr std::size_t kBlockSize = 64 * 1024;

    void* allocate(const std::size_t size, const std::size_t align) {
        // 块基址由 operator new[] 对齐到默认新分配对齐（≥16），节点对齐均 ≤ 此值，
        // 故只需在块内把偏移对齐到 align 即可。
        std::size_t aligned = (m_offset + align - 1) & ~(align - 1);
        if (m_block == nullptr || aligned + size > m_capacity) {
            const std::size_t block = std::max(kBlockSize, size + align);
            // 用 new[]（默认初始化，对 std::byte 即不初始化）而非 make_unique<byte[]>
            // （值初始化会把整块清零）——我们随后 placement-new 覆盖，清零纯属浪费。
            m_blocks.push_back(std::unique_ptr<std::byte[]>(new std::byte[block]));
            m_block    = m_blocks.back().get();
            m_capacity = block;
            m_offset   = 0;
            aligned    = 0;
        }
        std::byte* result = m_block + aligned;
        m_offset          = aligned + size;
        return result;
    }

    std::vector<std::unique_ptr<std::byte[]>> m_blocks;
    std::vector<Node*>                        m_nodes;
    std::byte*                                m_block    = nullptr;
    std::size_t                               m_capacity = 0;
    std::size_t                               m_offset   = 0;
};

}  // namespace hps
