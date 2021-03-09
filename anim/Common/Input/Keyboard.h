#pragma once
#include "KeyboardEvent.h"
#include <queue>

namespace input {
	class Keyboard
	{
	public:
		Keyboard();

		bool KeyWasReleased(const unsigned char keycode);
		bool KeyIsPressed(const unsigned char keycode);
		bool KeyBufferIsEmpty();
		bool CharBufferIsEmpty();

		KeyboardEvent ReadKey();
		unsigned char ReadChar();

		void OnKeyPressed(const unsigned char key);
		void OnKeyReleased(const unsigned char key);

		void OnChar(const unsigned char key);

		void EnableAutoRepeatKeys();
		void DisableAutoRepeatKeys();

		void EnableAutoRepeatChars();
		void DisableAutoRepeatChars();

		bool IsKeysAutoRepeat();
		bool IsCharsAutoRepeat();

		void Update();

	private:
		bool m_autoRepeatKeys = false;
		bool m_autoRepeatChars = false;

		bool m_keyStates[256];
		bool m_keyReleased[256];

		std::queue<KeyboardEvent> m_keyBuffer;
		std::queue<unsigned char> m_charBuffer;
	};
}
