#pragma once

// Undo/redo history for user actions.
//
// Two execution paths:
//   execute(ICommand*)     — for registry-owned toolbar/menu commands.
//                            Calls cmd->execute(); if cmd->undoable() the
//                            pointer is pushed onto the undo stack (the
//                            registry keeps the command alive for the session).
//   push(unique_ptr)       — for ad-hoc commands that are NOT in the registry
//                            (e.g. drag-finish operations). The stack takes
//                            ownership and calls nothing — the caller is
//                            responsible for having already applied the effect.
//
// Any new execute()/push() clears the redo stack.

#include "plugin/ICommand.hpp"

#include <memory>
#include <stack>
#include <vector>

namespace macad
{

    class CommandStack
    {
    public:
        // Execute a registry-owned command and, if undoable, push it.
        void execute(ICommand* cmd);

        // Take ownership of an already-applied command and push it for undo.
        void push(std::unique_ptr<ICommand> cmd);

        void undo();
        void redo();

        bool canUndo() const { return !m_undo.empty(); }
        bool canRedo() const { return !m_redo.empty(); }

        // Label of the next undo action, or empty string if none.
        std::string undoLabel() const;
        std::string redoLabel() const;

        void clear();

    private:
        // Entry holds either a non-owning pointer (registry command) or an
        // owning unique_ptr (ad-hoc command). Exactly one is non-null.
        struct Entry
        {
            ICommand*                  weak{ nullptr };  // non-owning
            std::unique_ptr<ICommand>  owned;            // owning

            ICommand* get() const { return owned ? owned.get() : weak; }
        };

        std::vector<Entry> m_undo;
        std::vector<Entry> m_redo;
    };

}  
