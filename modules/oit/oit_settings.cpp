#include "oit_settings.h"

#include "core/config/project_settings.h"

void oit_register_project_settings() {
	GLOBAL_DEF(PropertyInfo(Variant::BOOL, "rendering/oit/enabled"), false);
	GLOBAL_DEF(PropertyInfo(Variant::INT, "rendering/oit/slice_count", PROPERTY_HINT_ENUM, "64,128,256,512"), 128);
	GLOBAL_DEF(PropertyInfo(Variant::VECTOR2I, "rendering/oit/tile_size"), Vector2i(6, 6));
	GLOBAL_DEF(PropertyInfo(Variant::FLOAT, "rendering/oit/near_plane", PROPERTY_HINT_RANGE, "0.01,10.0,0.01"), 0.1);
	GLOBAL_DEF(PropertyInfo(Variant::FLOAT, "rendering/oit/far_plane", PROPERTY_HINT_RANGE, "1.0,10000.0,1.0"), 500.0);
	GLOBAL_DEF(PropertyInfo(Variant::FLOAT, "rendering/oit/linearization_factor", PROPERTY_HINT_RANGE, "0.001,4.0,0.001"), 0.5);
}
