/**************************************************************************/
/*  ggml_engine.h                                                         */
/**************************************************************************/
/* SPDX-License-Identifier: MIT                                           */
/* Copyright (c) 2026 K. S. Ernest (iFire) Lee                            */
/**************************************************************************/

#pragma once

#include "core/io/resource.h"
#include "core/object/ref_counted.h"

// The foundation Godot-facing surface for RFD 2242. Once #thirdparty/ggml/
// is imported, GgmlEngine wraps ggml_context and GgmlTensor wraps
// ggml_tensor*; the follow-up PR replaces the opaque `impl` pointer with
// the real struct forward-declarations.

class GgmlTensor : public RefCounted {
	GDCLASS(GgmlTensor, RefCounted);

protected:
	static void _bind_methods();

public:
	Vector<int> get_shape() const;

private:
	void *impl = nullptr;
};

class GgmlEngine : public Object {
	GDCLASS(GgmlEngine, Object);

protected:
	static void _bind_methods();

public:
	static GgmlEngine *get_singleton();

	GgmlEngine();
	~GgmlEngine();

private:
	static GgmlEngine *singleton;
	void *impl = nullptr;
};
