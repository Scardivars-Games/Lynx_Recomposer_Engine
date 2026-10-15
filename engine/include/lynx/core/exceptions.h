#pragma once
#include <exception>
#include <string>

namespace lynx::core{
    class BasicException : public std::exception {
    public:
        BasicException(const std::string& msg, const std::string& prefix)
            : m_message(msg), m_prefix(prefix) {}
        const char* what() const noexcept override {
            return m_message.c_str();
        }
        const char* prefix() const {
            return m_prefix.c_str();
        }
    private:
        std::string m_message;
        std::string m_prefix;
    };
}
