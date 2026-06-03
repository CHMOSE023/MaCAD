#pragma once

// A user-invokable action. Commands are the single entry point for toolbar
// buttons, menu items, plugin-contributed actions, and (later) undo/redo and
// parameter-triggered recompute. Milestone 1 uses them for built-in actions.

#include <string>

namespace macad 
{

    class ICommand 
    {
    public:
        virtual ~ICommand() = default;

        // Stable identifier, e.g. "macad.geometry.createBox".
        virtual std::string id()    const = 0;

        // Human-readable label for toolbars/menus.
        virtual std::string label() const = 0;

        // Whether the command can currently run (context dependent).
        virtual bool enabled() const { return true; }

        // Performs the action.
        virtual void execute() = 0;

        // Returns true if this command supports undo.
        // Commands that opt in must implement undo().
        virtual bool undoable() const { return false; }

        // Reverts the effect of the most recent execute(). Only called when
        // undoable() returns true. Must leave the application in the state it
        // was in before execute() ran.
        virtual void undo() {}
    };

}  
