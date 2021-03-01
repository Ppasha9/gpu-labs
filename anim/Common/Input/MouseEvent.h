#pragma once

namespace input {
	struct MousePoint
	{
		int x;
		int y;
	};

	class MouseEvent
	{
	public:
		enum class EventType
		{
			LPRESS,
			LRELEASE,
			RPRESS,
			RRELEASE,
			MPRESS,
			MRELEASE,
			WHEEL_UP,
			WHEEL_DOWN,
			MOVE,
			RAW_MOVE,
			INVALID
		};

		MouseEvent();
		MouseEvent(const EventType type, const int x, const int y);

		bool IsValid() const;

		EventType GetType() const;
		MousePoint GetPos() const;

		int GetPosX() const;
		int GetPosY() const;

	private:
		EventType m_type;
		int m_x;
		int m_y;
	};
}
