#include "pch.h"
#include "Keyboard.h"

using namespace input;

Keyboard::Keyboard()
{
	for (int i = 0; i < 256; i++) {
		m_keyStates[i] = false;
	}

	for (int i = 0; i < 256; i++)
		m_keyReleased[i] = false;
}

bool Keyboard::KeyIsPressed(const unsigned char keycode)
{
	return m_keyStates[keycode];
}

bool Keyboard::KeyBufferIsEmpty()
{
	return m_keyBuffer.empty();
}

bool Keyboard::CharBufferIsEmpty()
{
	return m_charBuffer.empty();
}

KeyboardEvent Keyboard::ReadKey()
{
	if (m_keyBuffer.empty())
	{
		return KeyboardEvent();
	}
	else
	{
		KeyboardEvent e = m_keyBuffer.front();
		m_keyBuffer.pop();
		return e;
	}
}

unsigned char Keyboard::ReadChar()
{
	if (m_charBuffer.empty())
	{
		return 0u;
	}
	else
	{
		unsigned char e = m_charBuffer.front();
		m_charBuffer.pop();
		return e; 
	}
}

void Keyboard::OnKeyPressed(const unsigned char key)
{
	m_keyStates[key] = true;
	m_keyBuffer.push(KeyboardEvent(KeyboardEvent::EventType::PRESS, key));
}

void Keyboard::OnKeyReleased(const unsigned char key)
{
	m_keyStates[key] = false;
	m_keyReleased[key] = true;
	m_keyBuffer.push(KeyboardEvent(KeyboardEvent::EventType::RELEASE, key));
}

void Keyboard::OnChar(const unsigned char key)
{
	m_charBuffer.push(key);
}

void Keyboard::EnableAutoRepeatKeys()
{
	m_autoRepeatKeys = true;
}

void Keyboard::DisableAutoRepeatKeys()
{
	m_autoRepeatKeys = false;
}

void Keyboard::EnableAutoRepeatChars()
{
	m_autoRepeatChars = true;
}

void Keyboard::DisableAutoRepeatChars()
{
	m_autoRepeatChars = false;
}

bool Keyboard::IsKeysAutoRepeat()
{
	return m_autoRepeatKeys;
}

bool Keyboard::IsCharsAutoRepeat()
{
	return m_autoRepeatChars;
}

void Keyboard::Update()
{
	for (int i = 0; i < 256; i++)
		m_keyReleased[i] = false;
}

bool Keyboard::KeyWasReleased(const unsigned char keycode)
{
	return m_keyReleased[keycode];
}
