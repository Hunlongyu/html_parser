#pragma once

#include <vector>
#include <string_view>
#include <memory>
#include <cstring>
#include <string>

namespace hps {

/**
 * @brief A memory pool for efficient string storage.
 * 
 * Manages a collection of memory blocks to store strings.
 * Provides stable string_views to the stored data.
 * Reduces memory fragmentation and allocation overhead compared to many small std::strings.
 */
class StringPool {
public:
    // Default block size: 4KB
    static constexpr size_t DEFAULT_BLOCK_SIZE = 4096;

    explicit StringPool(size_t block_size = DEFAULT_BLOCK_SIZE)
        : m_block_size(block_size) {
        allocate_new_block();
    }

    // Disable copy
    StringPool(const StringPool&) = delete;
    StringPool& operator=(const StringPool&) = delete;
    
    // Allow move
    StringPool(StringPool&&) = default;
    StringPool& operator=(StringPool&&) = default;

    /**
     * @brief Add a string to the pool.
     * @param str The string to add.
     * @return A string_view pointing to the copy in the pool.
     */
    std::string_view add(std::string_view str) {
        if (str.empty()) {
            return {};
        }

        // Handle large strings: allocate a dedicated block
        // If the string is larger than the standard block size, we don't want to
        // put it in a standard block as it might fill it up immediately.
        // Or if it's larger than the remaining space and also larger than block size.
        if (str.size() > m_block_size) {
            auto large_block = std::make_unique<char[]>(str.size());
            std::memcpy(large_block.get(), str.data(), str.size());
            std::string_view result(large_block.get(), str.size());
            m_blocks.push_back(std::move(large_block));
            // Note: we don't update m_current_block because this is a standalone block
            // m_current_block remains pointing to the current "filling" block
            return result;
        }

        // If not enough space in current block, allocate new one
        if (m_current_offset + str.size() > m_block_size) {
            allocate_new_block();
        }

        // Copy data to current block
        char* dest = m_current_block + m_current_offset;
        std::memcpy(dest, str.data(), str.size());
        m_current_offset += str.size();

        return {dest, str.size()};
    }
    
    std::string_view add(const std::string& str) {
        return add(std::string_view(str));
    }
    
    std::string_view add(const char* str) {
        return add(std::string_view(str));
    }

    /**
     * @brief Clear all stored strings and reset the pool.
     */
    void clear() {
        m_blocks.clear();
        m_current_block = nullptr;
        m_current_offset = 0;
        allocate_new_block();
    }

private:
    void allocate_new_block() {
        auto new_block = std::make_unique<char[]>(m_block_size);
        m_current_block = new_block.get();
        m_current_offset = 0;
        m_blocks.push_back(std::move(new_block));
    }

    size_t m_block_size;
    std::vector<std::unique_ptr<char[]>> m_blocks;
    char* m_current_block = nullptr;
    size_t m_current_offset = 0;
};

} // namespace hps
