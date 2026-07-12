#pragma once

#include "hrnpch.h"
#include "Heron/Core/Core.h"

namespace Heron {

	// Event types — each event class maps to one of these
	enum class EventType {
		None = 0,
		// Window events
		WindowClose, WindowResize, WindowFocus, WindowLostFocus, WindowMoved,
		// App events
		AppTick, AppUpdate, AppRender,
		// Key events
		KeyPressed, KeyReleased, KeyTyped,
		// Mouse events
		MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled
	};

	// Category bitmask for filtering events
	enum EventCategory {
		None                     = 0,
		EventCategoryApplication = BIT(0),
		EventCategoryInput       = BIT(1),
		EventCategoryKeyboard    = BIT(2),
		EventCategoryMouse       = BIT(3),
		EventCategoryMouseButton = BIT(4)
	};

	// Macros to reduce boilerplate in event subclasses
#define EVENT_CLASS_TYPE(type)                                              \
	static EventType GetStaticType() { return EventType::type; }           \
	virtual EventType GetEventType() const override { return GetStaticType(); } \
	virtual const char* GetName() const override { return #type; }

#define EVENT_CLASS_CATEGORY(category) \
	virtual int GetCategoryFlags() const override { return category; }

	// ---------- Base Event ----------
	class Event {
	public:
		virtual ~Event() = default;

		bool Handled = false;

		virtual EventType   GetEventType()     const = 0;
		virtual const char* GetName()          const = 0;
		virtual int         GetCategoryFlags() const = 0;

		virtual std::string ToString() const { return GetName(); }

		bool IsInCategory(EventCategory category) const {
			return GetCategoryFlags() & category;
		}
	};

	// ---------- Event Dispatcher ----------
	// Type-safe dispatch: tries to cast and call handler only if types match
	class EventDispatcher {
	public:
		EventDispatcher(Event& event) : m_Event(event) {}

		template<typename T, typename F>
		bool Dispatch(const F& func) {
			if (m_Event.GetEventType() == T::GetStaticType()) {
				m_Event.Handled |= func(static_cast<T&>(m_Event));
				return true;
			}
			return false;
		}

	private:
		Event& m_Event;
	};

	// For logging events
	inline std::ostream& operator<<(std::ostream& os, const Event& e) {
		return os << e.ToString();
	}

}
