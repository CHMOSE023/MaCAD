#include "plugin/CommandStack.hpp"

#include "core/Log.hpp"

namespace macad {

    void CommandStack::execute(ICommand* cmd)
    {
        if (!cmd) return;
        cmd->execute();
        if (cmd->undoable()) {
            m_redo.clear();
            m_undo.push_back(Entry{ cmd, nullptr });
            MACAD_LOG_DEBUG("CommandStack: pushed '{}' (undo depth {})",  cmd->label(), m_undo.size());
        }
    }

    void CommandStack::push(std::unique_ptr<ICommand> cmd)
    {
        if (!cmd) return;
        m_redo.clear();
        m_undo.push_back(Entry{ nullptr, std::move(cmd) });
        MACAD_LOG_DEBUG("CommandStack: pushed owned '{}' (undo depth {})",  m_undo.back().get()->label(), m_undo.size());
    }

    void CommandStack::undo()
    {
        if (m_undo.empty()) return;
        Entry e = std::move(m_undo.back());
        m_undo.pop_back();
        ICommand* cmd = e.get();
        MACAD_LOG_DEBUG("CommandStack: undo '{}'", cmd->label());
        cmd->undo();
        m_redo.push_back(std::move(e));
    }

    void CommandStack::redo() 
    {
        if (m_redo.empty()) return;
        Entry e = std::move(m_redo.back());
        m_redo.pop_back();
        ICommand* cmd = e.get();
        MACAD_LOG_DEBUG("CommandStack: redo '{}'", cmd->label());
        cmd->execute();
        m_undo.push_back(std::move(e));
    }

    std::string CommandStack::undoLabel() const
    {
        return m_undo.empty() ? std::string{} : m_undo.back().get()->label();
    }

    std::string CommandStack::redoLabel() const
    {
        return m_redo.empty() ? std::string{} : m_redo.back().get()->label();
    }

    void CommandStack::clear() 
    {
        m_undo.clear();
        m_redo.clear();
    }

}

