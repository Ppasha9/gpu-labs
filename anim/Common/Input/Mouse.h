#pragma once
#include "MouseEvent.h"
#include <queue>

namespace input {
	class Mouse
	{
	public:
		void OnLeftPressed(int x, int y);
		void OnLeftReleased(int x, int y);

		void OnRightPressed(int x, int y);
		void OnRightReleased(int x, int y);

		void OnMiddlePressed(int x, int y);
		void OnMiddleReleased(int x, int y);

		void OnWheelUp(int x, int y);
		void OnWheelDown(int x, int y);

		void OnMouseMove(int x, int y);
		void OnMouseMoveRaw(int x, int y);

		bool IsLeftDown();
		bool IsMiddleDown();
		bool IsRightDown();

		int GetPosX();
		int GetPosY();
		MousePoint GetPos();

		bool EventBufferIsEmpty();
		MouseEvent ReadEvent();

	private:
		std::queue<MouseEvent> m_eventBuffer;

		bool m_leftIsDown = false;
		bool m_rightIsDown = false;
		bool m_mbuttonDown = false;

		int m_x = 0;
		int m_y = 0;
	};
}
