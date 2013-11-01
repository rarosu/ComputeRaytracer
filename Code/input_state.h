#pragma once

#include "glm/glm.hpp"

struct input_state {
	struct mouse {
		glm::vec2 m_position;
	} m_mouse;

	struct keyboard {
		bool m_keys[256];
	} m_keyboard;
};