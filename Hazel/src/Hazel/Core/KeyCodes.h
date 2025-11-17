#pragma once

namespace GameEngine
{
	typedef enum class KeyCode : uint16_t
	{
		// From glfw3.h
		Space = 32,
		Apostrophe = 39, /* ' */
		Comma = 44, /* , */
		Minus = 45, /* - */
		Period = 46, /* . */
		Slash = 47, /* / */

		D0 = 48, /* 0 */
		D1 = 49, /* 1 */
		D2 = 50, /* 2 */
		D3 = 51, /* 3 */
		D4 = 52, /* 4 */
		D5 = 53, /* 5 */
		D6 = 54, /* 6 */
		D7 = 55, /* 7 */
		D8 = 56, /* 8 */
		D9 = 57, /* 9 */

		Semicolon = 59, /* ; */
		Equal = 61, /* = */

		A = 65,
		B = 66,
		C = 67,
		D = 68,
		E = 69,
		F = 70,
		G = 71,
		H = 72,
		I = 73,
		J = 74,
		K = 75,
		L = 76,
		M = 77,
		N = 78,
		O = 79,
		P = 80,
		Q = 81,
		R = 82,
		S = 83,
		T = 84,
		U = 85,
		V = 86,
		W = 87,
		X = 88,
		Y = 89,
		Z = 90,

		LeftBracket = 91,  /* [ */
		Backslash = 92,  /* \ */
		RightBracket = 93,  /* ] */
		GraveAccent = 96,  /* ` */

		World1 = 161, /* non-US #1 */
		World2 = 162, /* non-US #2 */

		/* Function keys */
		Escape = 256,
		Enter = 257,
		Tab = 258,
		Backspace = 259,
		Insert = 260,
		Delete = 261,
		Right = 262,
		Left = 263,
		Down = 264,
		Up = 265,
		PageUp = 266,
		PageDown = 267,
		Home = 268,
		End = 269,
		CapsLock = 280,
		ScrollLock = 281,
		NumLock = 282,
		PrintScreen = 283,
		Pause = 284,
		F1 = 290,
		F2 = 291,
		F3 = 292,
		F4 = 293,
		F5 = 294,
		F6 = 295,
		F7 = 296,
		F8 = 297,
		F9 = 298,
		F10 = 299,
		F11 = 300,
		F12 = 301,
		F13 = 302,
		F14 = 303,
		F15 = 304,
		F16 = 305,
		F17 = 306,
		F18 = 307,
		F19 = 308,
		F20 = 309,
		F21 = 310,
		F22 = 311,
		F23 = 312,
		F24 = 313,
		F25 = 314,

		/* Keypad */
		KP0 = 320,
		KP1 = 321,
		KP2 = 322,
		KP3 = 323,
		KP4 = 324,
		KP5 = 325,
		KP6 = 326,
		KP7 = 327,
		KP8 = 328,
		KP9 = 329,
		KPDecimal = 330,
		KPDivide = 331,
		KPMultiply = 332,
		KPSubtract = 333,
		KPAdd = 334,
		KPEnter = 335,
		KPEqual = 336,

		LeftShift = 340,
		LeftControl = 341,
		LeftAlt = 342,
		LeftSuper = 343,
		RightShift = 344,
		RightControl = 345,
		RightAlt = 346,
		RightSuper = 347,
		Menu = 348
	} Key;

	enum class KeyState
	{
		None = -1,
		Pressed,
		Held,
		Released
	};

	enum class CursorMode
	{
		Normal = 0,
		Hidden = 1,
		Locked = 2
	};

	typedef enum class MouseButton : uint16_t
	{
		Button0 = 0,
		Button1 = 1,
		Button2 = 2,
		Button3 = 3,
		Button4 = 4,
		Button5 = 5,
		Left = Button0,
		Right = Button1,
		Middle = Button2
	} Button;


	inline std::ostream& operator<<(std::ostream& os, KeyCode keyCode)
	{
		os << static_cast<int32_t>(keyCode);
		return os;
	}

	inline std::ostream& operator<<(std::ostream& os, MouseButton button)
	{
		os << static_cast<int32_t>(button);
		return os;
	}
}

// From glfw3.h
#define HZ_KEY_SPACE           ::GameEngine::Key::Space
#define HZ_KEY_APOSTROPHE      ::GameEngine::Key::Apostrophe    /* ' */
#define HZ_KEY_COMMA           ::GameEngine::Key::Comma         /* , */
#define HZ_KEY_MINUS           ::GameEngine::Key::Minus         /* - */
#define HZ_KEY_PERIOD          ::GameEngine::Key::Period        /* . */
#define HZ_KEY_SLASH           ::GameEngine::Key::Slash         /* / */
#define HZ_KEY_0               ::GameEngine::Key::D0
#define HZ_KEY_1               ::GameEngine::Key::D1
#define HZ_KEY_2               ::GameEngine::Key::D2
#define HZ_KEY_3               ::GameEngine::Key::D3
#define HZ_KEY_4               ::GameEngine::Key::D4
#define HZ_KEY_5               ::GameEngine::Key::D5
#define HZ_KEY_6               ::GameEngine::Key::D6
#define HZ_KEY_7               ::GameEngine::Key::D7
#define HZ_KEY_8               ::GameEngine::Key::D8
#define HZ_KEY_9               ::GameEngine::Key::D9
#define HZ_KEY_SEMICOLON       ::GameEngine::Key::Semicolon     /* ; */
#define HZ_KEY_EQUAL           ::GameEngine::Key::Equal         /* = */
#define HZ_KEY_A               ::GameEngine::Key::A
#define HZ_KEY_B               ::GameEngine::Key::B
#define HZ_KEY_C               ::GameEngine::Key::C
#define HZ_KEY_D               ::GameEngine::Key::D
#define HZ_KEY_E               ::GameEngine::Key::E
#define HZ_KEY_F               ::GameEngine::Key::F
#define HZ_KEY_G               ::GameEngine::Key::G
#define HZ_KEY_H               ::GameEngine::Key::H
#define HZ_KEY_I               ::GameEngine::Key::I
#define HZ_KEY_J               ::GameEngine::Key::J
#define HZ_KEY_K               ::GameEngine::Key::K
#define HZ_KEY_L               ::GameEngine::Key::L
#define HZ_KEY_M               ::GameEngine::Key::M
#define HZ_KEY_N               ::GameEngine::Key::N
#define HZ_KEY_O               ::GameEngine::Key::O
#define HZ_KEY_P               ::GameEngine::Key::P
#define HZ_KEY_Q               ::GameEngine::Key::Q
#define HZ_KEY_R               ::GameEngine::Key::R
#define HZ_KEY_S               ::GameEngine::Key::S
#define HZ_KEY_T               ::GameEngine::Key::T
#define HZ_KEY_U               ::GameEngine::Key::U
#define HZ_KEY_V               ::GameEngine::Key::V
#define HZ_KEY_W               ::GameEngine::Key::W
#define HZ_KEY_X               ::GameEngine::Key::X
#define HZ_KEY_Y               ::GameEngine::Key::Y
#define HZ_KEY_Z               ::GameEngine::Key::Z
#define HZ_KEY_LEFT_BRACKET    ::GameEngine::Key::LeftBracket   /* [ */
#define HZ_KEY_BACKSLASH       ::GameEngine::Key::Backslash     /* \ */
#define HZ_KEY_RIGHT_BRACKET   ::GameEngine::Key::RightBracket  /* ] */
#define HZ_KEY_GRAVE_ACCENT    ::GameEngine::Key::GraveAccent   /* ` */
#define HZ_KEY_WORLD_1         ::GameEngine::Key::World1        /* non-US #1 */
#define HZ_KEY_WORLD_2         ::GameEngine::Key::World2        /* non-US #2 */

/* Function keys */
#define HZ_KEY_ESCAPE          ::GameEngine::Key::Escape
#define HZ_KEY_ENTER           ::GameEngine::Key::Enter
#define HZ_KEY_TAB             ::GameEngine::Key::Tab
#define HZ_KEY_BACKSPACE       ::GameEngine::Key::Backspace
#define HZ_KEY_INSERT          ::GameEngine::Key::Insert
#define HZ_KEY_DELETE          ::GameEngine::Key::Delete
#define HZ_KEY_RIGHT           ::GameEngine::Key::Right
#define HZ_KEY_LEFT            ::GameEngine::Key::Left
#define HZ_KEY_DOWN            ::GameEngine::Key::Down
#define HZ_KEY_UP              ::GameEngine::Key::Up
#define HZ_KEY_PAGE_UP         ::GameEngine::Key::PageUp
#define HZ_KEY_PAGE_DOWN       ::GameEngine::Key::PageDown
#define HZ_KEY_HOME            ::GameEngine::Key::Home
#define HZ_KEY_END             ::GameEngine::Key::End
#define HZ_KEY_CAPS_LOCK       ::GameEngine::Key::CapsLock
#define HZ_KEY_SCROLL_LOCK     ::GameEngine::Key::ScrollLock
#define HZ_KEY_NUM_LOCK        ::GameEngine::Key::NumLock
#define HZ_KEY_PRINT_SCREEN    ::GameEngine::Key::PrintScreen
#define HZ_KEY_PAUSE           ::GameEngine::Key::Pause
#define HZ_KEY_F1              ::GameEngine::Key::F1
#define HZ_KEY_F2              ::GameEngine::Key::F2
#define HZ_KEY_F3              ::GameEngine::Key::F3
#define HZ_KEY_F4              ::GameEngine::Key::F4
#define HZ_KEY_F5              ::GameEngine::Key::F5
#define HZ_KEY_F6              ::GameEngine::Key::F6
#define HZ_KEY_F7              ::GameEngine::Key::F7
#define HZ_KEY_F8              ::GameEngine::Key::F8
#define HZ_KEY_F9              ::GameEngine::Key::F9
#define HZ_KEY_F10             ::GameEngine::Key::F10
#define HZ_KEY_F11             ::GameEngine::Key::F11
#define HZ_KEY_F12             ::GameEngine::Key::F12
#define HZ_KEY_F13             ::GameEngine::Key::F13
#define HZ_KEY_F14             ::GameEngine::Key::F14
#define HZ_KEY_F15             ::GameEngine::Key::F15
#define HZ_KEY_F16             ::GameEngine::Key::F16
#define HZ_KEY_F17             ::GameEngine::Key::F17
#define HZ_KEY_F18             ::GameEngine::Key::F18
#define HZ_KEY_F19             ::GameEngine::Key::F19
#define HZ_KEY_F20             ::GameEngine::Key::F20
#define HZ_KEY_F21             ::GameEngine::Key::F21
#define HZ_KEY_F22             ::GameEngine::Key::F22
#define HZ_KEY_F23             ::GameEngine::Key::F23
#define HZ_KEY_F24             ::GameEngine::Key::F24
#define HZ_KEY_F25             ::GameEngine::Key::F25

/* Keypad */
#define HZ_KEY_KP_0            ::GameEngine::Key::KP0
#define HZ_KEY_KP_1            ::GameEngine::Key::KP1
#define HZ_KEY_KP_2            ::GameEngine::Key::KP2
#define HZ_KEY_KP_3            ::GameEngine::Key::KP3
#define HZ_KEY_KP_4            ::GameEngine::Key::KP4
#define HZ_KEY_KP_5            ::GameEngine::Key::KP5
#define HZ_KEY_KP_6            ::GameEngine::Key::KP6
#define HZ_KEY_KP_7            ::GameEngine::Key::KP7
#define HZ_KEY_KP_8            ::GameEngine::Key::KP8
#define HZ_KEY_KP_9            ::GameEngine::Key::KP9
#define HZ_KEY_KP_DECIMAL      ::GameEngine::Key::KPDecimal
#define HZ_KEY_KP_DIVIDE       ::GameEngine::Key::KPDivide
#define HZ_KEY_KP_MULTIPLY     ::GameEngine::Key::KPMultiply
#define HZ_KEY_KP_SUBTRACT     ::GameEngine::Key::KPSubtract
#define HZ_KEY_KP_ADD          ::GameEngine::Key::KPAdd
#define HZ_KEY_KP_ENTER        ::GameEngine::Key::KPEnter
#define HZ_KEY_KP_EQUAL        ::GameEngine::Key::KPEqual

#define HZ_KEY_LEFT_SHIFT      ::GameEngine::Key::LeftShift
#define HZ_KEY_LEFT_CONTROL    ::GameEngine::Key::LeftControl
#define HZ_KEY_LEFT_ALT        ::GameEngine::Key::LeftAlt
#define HZ_KEY_LEFT_SUPER      ::GameEngine::Key::LeftSuper
#define HZ_KEY_RIGHT_SHIFT     ::GameEngine::Key::RightShift
#define HZ_KEY_RIGHT_CONTROL   ::GameEngine::Key::RightControl
#define HZ_KEY_RIGHT_ALT       ::GameEngine::Key::RightAlt
#define HZ_KEY_RIGHT_SUPER     ::GameEngine::Key::RightSuper
#define HZ_KEY_MENU            ::GameEngine::Key::Menu

// Mouse
#define HZ_MOUSE_BUTTON_LEFT    ::GameEngine::Button::Left
#define HZ_MOUSE_BUTTON_RIGHT   ::GameEngine::Button::Right
#define HZ_MOUSE_BUTTON_MIDDLE  ::GameEngine::Button::Middle


