/*
 * gkpro @ 2020-10-25
 *   Nexus Library
 *     Text based User Interface - header
 *     UI Action
 */

#pragma once

namespace nxs {

/**
 * @brief An interface for an action that can be executed
 */
class UIAction {
public:
    virtual ~UIAction() = default;
    virtual void execute() = 0;
};


/**
 * @brief An action that calls a function
 */
template <class Callable>
class UICallback : public UIAction {
public:
    UICallback(Callable func) : m_func(func) {}
    ~UICallback() = default;
    void execute() override { m_func(); }

private:
    Callable m_func;
};


} // namespace nxs
