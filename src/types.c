#include "types.h"

bool rect_contains(RRect rect, RVec2 point) {
	if (point.x < rect.x) return false;
	if (point.y < rect.y) return false;

	if (point.x > rect.x + rect.width) return false;
	if (point.y > rect.y + rect.height) return false;

	return true;
}
