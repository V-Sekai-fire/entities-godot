#pragma once

#include "scene/resources/compositor.h"

class OITCompositorEffect : public CompositorEffect {
	GDCLASS(OITCompositorEffect, CompositorEffect);

	int slice_count = 128;
	Vector2i tile_size = Vector2i(6, 6);
	float near_plane = 0.1f;
	float far_plane = 500.0f;
	float linearization_factor = 0.5f;

protected:
	static void _bind_methods();

public:
	void set_slice_count(int p_slices);
	int get_slice_count() const;

	void set_tile_size(const Vector2i &p_size);
	Vector2i get_tile_size() const;

	void set_near_plane(float p_near);
	float get_near_plane() const;

	void set_far_plane(float p_far);
	float get_far_plane() const;

	void set_linearization_factor(float p_factor);
	float get_linearization_factor() const;

	OITCompositorEffect();
};
