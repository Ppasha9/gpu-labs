#include "pch.h"
#include "KeyboardEvent.h"

using namespace input;

KeyboardEvent::KeyboardEvent()
	: m_type(EventType::INVALID)
	, m_key(0u)
{
}

KeyboardEvent::KeyboardEvent(const EventType type, const unsigned char key)
	: m_type(type)
	, m_key(key)
{
}

bool KeyboardEvent::IsPress() const
{
	return m_type == EventType::PRESS;
}

bool KeyboardEvent::IsRelease() const
{
	return m_type == EventType::RELEASE;
}

bool KeyboardEvent::IsValid() const
{
	return m_type != EventType::INVALID;
}

unsigned char KeyboardEvent::GetKeyCode() const
{
	return m_key;
}
