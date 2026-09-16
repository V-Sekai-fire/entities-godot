#include "oit_compositor_effect.h"

#include "core/object/class_db.h"

OITCompositorEffect::OITCompositorEffect() {
	set_effect_callback_type(EFFECT_CALLBACK_TYPE_PRE_TRANSPARENT);
	set_needs_motion_vectors(false);
	set_needs_normal_roughness(false);
}

void OITCompositorEffect::set_slice_count(int p_slices) {
	slice_count = p_slices;
}
int OITCompositorEffect::get_slice_count() const {
	return slice_count;
}

void OITCompositorEffect::set_tile_size(const Vector2i &p_size) {
	tile_size = p_size;
}
Vector2i OITCompositorEffect::get_tile_size() const {
	return tile_size;
}

void OITCompositorEffect::set_near_plane(float p_near) {
	near_plane = p_near;
}
float OITCompositorEffect::get_near_plane() const {
	return near_plane;
}

void OITCompositorEffect::set_far_plane(float p_far) {
	far_plane = p_far;
}
float OITCompositorEffect::get_far_plane() const {
	return far_plane;
}

void OITCompositorEffect::set_linearization_factor(float p_factor) {
	linearization_factor = p_factor;
}
float OITCompositorEffect::get_linearization_factor() const {
	return linearization_factor;
}

void OITCompositorEffect::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_slice_count", "slices"), &OITCompositorEffect::set_slice_count);
	ClassDB::bind_method(D_METHOD("get_slice_count"), &OITCompositorEffect::get_slice_count);
	ClassDB::bind_method(D_METHOD("set_tile_size", "size"), &OITCompositorEffect::set_tile_size);
	ClassDB::bind_method(D_METHOD("get_tile_size"), &OITCompositorEffect::get_tile_size);
	ClassDB::bind_method(D_METHOD("set_near_plane", "near"), &OITCompositorEffect::set_near_plane);
	ClassDB::bind_method(D_METHOD("get_near_plane"), &OITCompositorEffect::get_near_plane);
	ClassDB::bind_method(D_METHOD("set_far_plane", "far"), &OITCompositorEffect::set_far_plane);
	ClassDB::bind_method(D_METHOD("get_far_plane"), &OITCompositorEffect::get_far_plane);
	ClassDB::bind_method(D_METHOD("set_linearization_factor", "factor"), &OITCompositorEffect::set_linearization_factor);
	ClassDB::bind_method(D_METHOD("get_linearization_factor"), &OITCompositorEffect::get_linearization_factor);

	ADD_PROPERTY(PropertyInfo(Variant::INT, "slice_count", PROPERTY_HINT_ENUM, "64,128,256,512"), "set_slice_count", "get_slice_count");
	ADD_PROPERTY(PropertyInfo(Variant::VECTOR2I, "tile_size"), "set_tile_size", "get_tile_size");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "near_plane", PROPERTY_HINT_RANGE, "0.01,10.0,0.01"), "set_near_plane", "get_near_plane");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "far_plane", PROPERTY_HINT_RANGE, "1.0,10000.0,1.0"), "set_far_plane", "get_far_plane");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "linearization_factor", PROPERTY_HINT_RANGE, "0.001,4.0,0.001"), "set_linearization_factor", "get_linearization_factor");
}
