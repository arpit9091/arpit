/**************************************************************************/
/*  cylinder_shape_3d.cpp                                                 */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "cylinder_shape_3d.h"

#include "scene/resources/mesh.h"
#include "servers/physics_server_3d.h"

Vector<Vector3> CylinderShape3D::get_debug_mesh_lines() const {
	float c_radius = get_radius();
	float c_height = get_height();

	Vector<Vector3> points;

	Vector3 d(0, c_height * 0.5, 0);
	for (int i = 0; i < 360; i++) {
		float ra = Math::deg_to_rad((float)i);
		float rb = Math::deg_to_rad((float)i + 1);
		Point2 a = Vector2(Math::sin(ra), Math::cos(ra)) * c_radius;
		Point2 b = Vector2(Math::sin(rb), Math::cos(rb)) * c_radius;

		points.push_back(Vector3(a.x, 0, a.y) + d);
		points.push_back(Vector3(b.x, 0, b.y) + d);

		points.push_back(Vector3(a.x, 0, a.y) - d);
		points.push_back(Vector3(b.x, 0, b.y) - d);

		if (i % 90 == 0) {
			points.push_back(Vector3(a.x, 0, a.y) + d);
			points.push_back(Vector3(a.x, 0, a.y) - d);
		}
	}

	return points;
}

Ref<ArrayMesh> CylinderShape3D::get_debug_arraymesh_faces(const Color &p_modulate) const {
	int prevrow, thisrow, point;
	float x, y, z, u, v;

	Vector<Vector3> points;
	Vector<Color> colors;
	Vector<int> indices;
	point = 0;

	const int radial_segments = 32;

	thisrow = 0;
	prevrow = 0;
	for (int j = 0; j <= 1; j++) {
		v = j;

		y = height * v;
		y = (height * 0.5) - y;

		for (int i = 0; i <= radial_segments; i++) {
			u = i;
			u /= radial_segments;

			x = sin(u * Math_TAU);
			z = cos(u * Math_TAU);

			Vector3 p = Vector3(x * radius, y, z * radius);
			points.push_back(p);
			colors.push_back(p_modulate);
			point++;

			if (i > 0 && j > 0) {
				indices.push_back(prevrow + i - 1);
				indices.push_back(prevrow + i);
				indices.push_back(thisrow + i - 1);

				indices.push_back(prevrow + i);
				indices.push_back(thisrow + i);
				indices.push_back(thisrow + i - 1);
			}
		}

		prevrow = thisrow;
		thisrow = point;
	}

	// Add top.
	y = height * 0.5;

	thisrow = point;
	points.push_back(Vector3(0.0, y, 0.0));
	colors.push_back(p_modulate);
	point++;

	for (int i = 0; i <= radial_segments; i++) {
		float r = i;
		r /= radial_segments;

		x = sin(r * Math_TAU);
		z = cos(r * Math_TAU);

		u = ((x + 1.0) * 0.25);
		v = 0.5 + ((z + 1.0) * 0.25);

		Vector3 p = Vector3(x * radius, y, z * radius);
		points.push_back(p);
		colors.push_back(p_modulate);
		point++;

		if (i > 0) {
			indices.push_back(thisrow);
			indices.push_back(point - 1);
			indices.push_back(point - 2);
		}
	}

	// Add bottom.
	y = height * -0.5;

	thisrow = point;
	points.push_back(Vector3(0.0, y, 0.0));
	colors.push_back(p_modulate);
	point++;

	for (int i = 0; i <= radial_segments; i++) {
		float r = i;
		r /= radial_segments;

		x = sin(r * Math_TAU);
		z = cos(r * Math_TAU);

		u = 0.5 + ((x + 1.0) * 0.25);
		v = 1.0 - ((z + 1.0) * 0.25);

		Vector3 p = Vector3(x * radius, y, z * radius);
		points.push_back(p);
		colors.push_back(p_modulate);
		point++;

		if (i > 0) {
			indices.push_back(thisrow);
			indices.push_back(point - 2);
			indices.push_back(point - 1);
		}
	}

	Ref<ArrayMesh> mesh = memnew(ArrayMesh);
	Array a;
	a.resize(Mesh::ARRAY_MAX);
	a[RS::ARRAY_VERTEX] = points;
	a[RS::ARRAY_COLOR] = colors;
	a[RS::ARRAY_INDEX] = indices;
	mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, a);

	return mesh;
}

real_t CylinderShape3D::get_enclosing_radius() const {
	return Vector2(radius, height * 0.5).length();
}

void CylinderShape3D::_update_shape() {
	Dictionary d;
	d["radius"] = radius;
	d["height"] = height;
	PhysicsServer3D::get_singleton()->shape_set_data(get_shape(), d);
	Shape3D::_update_shape();
}

void CylinderShape3D::set_radius(float p_radius) {
	ERR_FAIL_COND_MSG(p_radius < 0, "CylinderShape3D radius cannot be negative.");
	radius = p_radius;
	_update_shape();
	emit_changed();
}

float CylinderShape3D::get_radius() const {
	return radius;
}

void CylinderShape3D::set_height(float p_height) {
	ERR_FAIL_COND_MSG(p_height < 0, "CylinderShape3D height cannot be negative.");
	height = p_height;
	_update_shape();
	emit_changed();
}

float CylinderShape3D::get_height() const {
	return height;
}

void CylinderShape3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_radius", "radius"), &CylinderShape3D::set_radius);
	ClassDB::bind_method(D_METHOD("get_radius"), &CylinderShape3D::get_radius);
	ClassDB::bind_method(D_METHOD("set_height", "height"), &CylinderShape3D::set_height);
	ClassDB::bind_method(D_METHOD("get_height"), &CylinderShape3D::get_height);

	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "height", PROPERTY_HINT_RANGE, "0.001,100,0.001,or_greater,suffix:m"), "set_height", "get_height");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "radius", PROPERTY_HINT_RANGE, "0.001,100,0.001,or_greater,suffix:m"), "set_radius", "get_radius");
}

CylinderShape3D::CylinderShape3D() :
		Shape3D(PhysicsServer3D::get_singleton()->shape_create(PhysicsServer3D::SHAPE_CYLINDER)) {
	_update_shape();
}
