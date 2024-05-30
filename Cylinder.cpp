
#include "Cylinder.h"
#include <math.h>

float Cylinder::intersect(const glm::vec3 p0, const glm::vec3 dir) {
	float xdiff = p0.x - center.x;
	float zdiff = p0.z - center.z;

	float a = dir.x * dir.x + dir.z * dir.z;
	float b = 2.0 * (dir.x * (xdiff) + dir.z * (zdiff));
	float c = xdiff * xdiff + zdiff * zdiff - radius * radius;

	float discriminant = b * b - 4. * a * c;

	if (discriminant < 0.001) return -1.0;

	float t1 = ( - b + sqrt(discriminant)) / (2. * a);
	float t2 = ( - b - sqrt(discriminant)) / (2. * a);

    float t_enter = fmin(t1, t2);
    float t_exit = fmax(t1, t2);

    glm::vec3 enter_point = p0 + t_enter * dir;
    glm::vec3 exit_point = p0 + t_exit * dir;

	if (enter_point.y >= center.y && enter_point.y <= center.y + height) {
		return t_enter;
	}
	else if (exit_point.y >= center.y && exit_point.y <= center.y + height) {
		return t_exit;
	}
	else return -1.0;
}

glm::vec3 Cylinder::normal(glm::vec3 p) {
	glm::vec3 n = glm::vec3(p.x - center.x, 0, p.z - center.z);
	n = glm::normalize(n);
	return n;
}
