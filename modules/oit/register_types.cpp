#include "register_types.h"

#include "core/object/class_db.h"

#include "oit_compositor_effect.h"
#include "oit_settings.h"

void initialize_oit_module(ModuleInitializationLevel p_level) {
	if (p_level == MODULE_INITIALIZATION_LEVEL_SERVERS) {
		oit_register_project_settings();
	}
	if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
		GDREGISTER_CLASS(OITCompositorEffect);
	}
}

void uninitialize_oit_module(ModuleInitializationLevel p_level) {
	(void)p_level;
}
