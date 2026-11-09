#include "resource_saver_elf.h"
#include "../fast_cast.hpp"
#include "../register_types.h"
#include "script_elf.h"
#include "script_language_elf.h"
#include <godot_cpp/classes/file_access.hpp>

Error ResourceFormatSaverELF::_save(const Ref<Resource> &p_resource, const String &p_path, uint32_t p_flags) {
	Ref<ELFScript> elf_model = fast_cast_to<ELFScript>(p_resource.ptr());
	if (elf_model.is_null()) {
		return Error::OK;
	}
	// p_path is the same file the resource was already loaded from (the editor's
	// interactive Save on an open ELF script): the binary on disk is already
	// correct, so revert in-memory state to it instead of overwriting.
	if (elf_model->get_path() == p_path) {
		elf_model->set_file(p_path);
		elf_model->reload(true);
		return Error::OK;
	}
	// p_path is a *different* file -- this is ResourceSaver::save() being used to
	// copy/export the resource (e.g. the editor's --export-pack pipeline routes any
	// customized/"modified" resource through here). The old code above did nothing
	// here, so the exported entry ended up with no payload at all: only the
	// resource's own class-list/metadata was ever recorded, not its ELF bytes.
	// Write the real content so an export of this resource is never silently empty.
	const PackedByteArray &content = elf_model->get_content();
	Ref<FileAccess> f = FileAccess::open(p_path, FileAccess::ModeFlags::WRITE);
	ERR_FAIL_COND_V_MSG(f.is_null(), FileAccess::get_open_error(), "ELF saver: failed to open " + p_path + " for writing");
	f->store_buffer(content.ptr(), content.size());
	f->close();
	return Error::OK;
}
Error ResourceFormatSaverELF::_set_uid(const String &p_path, int64_t p_uid) {
	return Error::OK;
}
bool ResourceFormatSaverELF::_recognize(const Ref<Resource> &p_resource) const {
	return fast_cast_to<ELFScript>(p_resource.ptr()) != nullptr;
}
PackedStringArray ResourceFormatSaverELF::_get_recognized_extensions(const Ref<Resource> &p_resource) const {
	PackedStringArray array;
	if (fast_cast_to<ELFScript>(p_resource.ptr()) == nullptr)
		return array;
	array.push_back("elf");
	return array;
}
bool ResourceFormatSaverELF::_recognize_path(const Ref<Resource> &p_resource, const String &p_path) const {
	return fast_cast_to<ELFScript>(p_resource.ptr()) != nullptr;
}
