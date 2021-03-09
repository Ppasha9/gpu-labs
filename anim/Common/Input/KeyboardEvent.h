#pragma once

namespace input {
    class KeyboardEvent {
    public:
        enum class EventType {
            PRESS,
            RELEASE,
            INVALID
        };

        KeyboardEvent();
        KeyboardEvent(const EventType type, const unsigned char key);

        bool IsPress() const;
        bool IsRelease() const;
        bool IsValid() const;

        unsigned char GetKeyCode() const;

    private:
        EventType m_type;
        unsigned char m_key;
    };
}
