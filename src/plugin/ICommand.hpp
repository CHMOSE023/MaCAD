#pragma once

// A user-invokable action. Commands are the single entry point for toolbar
// buttons, menu items, plugin-contributed actions, and (later) undo/redo and
// parameter-triggered recompute. Milestone 1 uses them for built-in actions.

#include <string>

namespace macad {

    class ICommand {
    public:
        virtual ~ICommand() = default;

        // Stable identifier, e.g. "macad.geometry.createBox".
        virtual std::string id() const = 0;

        // Human-readable label for toolbars/menus.
        virtual std::string label() const = 0;

        // Whether the command can currently run (context dependent).
        virtual bool enabled() const { return true; }

        // Performs the action.
        virtual void execute() = 0;
    };

} // namespace macad
