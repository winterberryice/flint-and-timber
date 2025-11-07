#pragma once

#include <SDL3/SDL.h>

namespace flint
{
    // Manages game state and determines which systems should be active
    class GameState
    {
    public:
        GameState() = default;

        // State queries
        bool should_process_player_input() const { return !m_inventory_open; }
        bool should_allow_block_interaction() const { return !m_inventory_open; }
        bool is_inventory_open() const { return m_inventory_open; }
        bool should_use_relative_mouse() const { return !m_inventory_open; }

        // State changes
        void toggle_inventory()
        {
            m_inventory_open = !m_inventory_open;
        }

        void open_inventory()
        {
            m_inventory_open = true;
        }

        void close_inventory()
        {
            m_inventory_open = false;
        }

    private:
        bool m_inventory_open = false;
        // Future: add more states like pause menu, crafting, etc.
    };

} // namespace flint
