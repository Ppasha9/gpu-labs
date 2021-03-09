#include "pch.h"
#include "Mouse.h"

using namespace input;

void Mouse::OnLeftPressed(int x, int y)
{
	m_leftIsDown = true;
	MouseEvent me(MouseEvent::EventType::LPRESS, x, y);
	m_eventBuffer.push(me);
}

void Mouse::OnLeftReleased(int x, int y)
{
	m_leftIsDown = false;
	m_eventBuffer.push(MouseEvent(MouseEvent::EventType::LRELEASE, x, y));
}

void Mouse::OnRightPressed(int x, int y)
{
	m_rightIsDown = true;
	m_eventBuffer.push(MouseEvent(MouseEvent::EventType::RPRESS, x, y));
}

void Mouse::OnRightReleased(int x, int y)
{
	m_rightIsDown = false;
	m_eventBuffer.push(MouseEvent(MouseEvent::EventType::RRELEASE, x, y));
}

void Mouse::OnMiddlePressed(int x, int y)
{
	m_mbuttonDown = true;
	m_eventBuffer.push(MouseEvent(MouseEvent::EventType::MPRESS, x, y));
}

void Mouse::OnMiddleReleased(int x, int y)
{
	m_mbuttonDown = false;
	m_eventBuffer.push(MouseEvent(MouseEvent::EventType::MRELEASE, x, y));
}

void Mouse::OnWheelUp(int x, int y)
{
	m_eventBuffer.push(MouseEvent(MouseEvent::EventType::WHEEL_UP, x, y));
}

void Mouse::OnWheelDown(int x, int y)
{
	m_eventBuffer.push(MouseEvent(MouseEvent::EventType::WHEEL_DOWN, x, y));
}

void Mouse::OnMouseMove(int x, int y)
{
	m_x = x;
	m_y = y;
	m_eventBuffer.push(MouseEvent(MouseEvent::EventType::MOVE, x, y));
}

void Mouse::OnMouseMoveRaw(int x, int y)
{
	m_eventBuffer.push(MouseEvent(MouseEvent::EventType::RAW_MOVE, x, y));
}

bool Mouse::IsLeftDown()
{
	return m_leftIsDown;
}

bool Mouse::IsMiddleDown()
{
	return m_mbuttonDown;
}

bool Mouse::IsRightDown()
{
	return m_rightIsDown;
}

int Mouse::GetPosX()
{
	return m_x;
}

int Mouse::GetPosY()
{
	return m_y;
}

MousePoint Mouse::GetPos()
{
	return{ m_x, m_y };
}

bool Mouse::EventBufferIsEmpty()
{
	return m_eventBuffer.empty();
}

MouseEvent Mouse::ReadEvent()
{
	if (m_eventBuffer.empty())
	{
		return MouseEvent();
	}
	else
	{
		MouseEvent e = m_eventBuffer.front();
		m_eventBuffer.pop();
		return e;
	}
}
