/**************************************************************************/
/*  test_order_independent_transparency.h                                 */
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

#pragma once

#include "../order_independent_transparency_compositor_effect.h"
#include "../order_independent_transparency_geometry_relay.h"
#include "../order_independent_transparency_settings.h"

#include "core/config/project_settings.h"
#include "core/object/class_db.h"
#include "scene/resources/compositor.h"

#include "tests/test_macros.h"

#include <thirdparty/witness-cpp/include/witness/ladder.h>

#include <functional>

namespace TestOrderIndependentTransparency {

using CompositorEffectOIT = OrderIndependentTransparencyCompositorEffect;
using RelayOIT = OrderIndependentTransparencyGeometryRelay;

static bool property_list_has(const List<PropertyInfo> &p_list, const StringName &p_name, Variant::Type p_type) {
	for (const PropertyInfo &pi : p_list) {
		if (pi.name == p_name) {
			return pi.type == p_type;
		}
	}
	return false;
}

static bool signal_list_has(const List<MethodInfo> &p_list, const StringName &p_name, int p_param_count) {
	for (const MethodInfo &mi : p_list) {
		if (mi.name == p_name) {
			return mi.arguments.size() == p_param_count;
		}
	}
	return false;
}

// -------- exact-value tests: registration surface --------

TEST_CASE("[OIT] Compositor effect class registered with correct parent") {
	CHECK(ClassDB::class_exists("OrderIndependentTransparencyCompositorEffect"));
	CHECK(ClassDB::get_parent_class("OrderIndependentTransparencyCompositorEffect") == StringName("CompositorEffect"));
	CHECK(ClassDB::is_parent_class("OrderIndependentTransparencyCompositorEffect", "CompositorEffect"));
	CHECK(ClassDB::is_parent_class("OrderIndependentTransparencyCompositorEffect", "Resource"));
	CHECK_FALSE(ClassDB::is_parent_class("OrderIndependentTransparencyCompositorEffect", "Node"));
}

TEST_CASE("[OIT] Compositor effect property surface is exact") {
	List<PropertyInfo> props;
	ClassDB::get_property_list("OrderIndependentTransparencyCompositorEffect", &props, true);
	CHECK(property_list_has(props, "ldr_scale", Variant::FLOAT));
	CHECK(property_list_has(props, "msaa_enabled", Variant::BOOL));
	CHECK(property_list_has(props, "rt_format", Variant::INT));
	CHECK(property_list_has(props, "average_depth_complexity", Variant::FLOAT));
	CHECK(property_list_has(props, "debug_overlay_enabled", Variant::BOOL));
	CHECK_FALSE(property_list_has(props, "oit_enabled", Variant::BOOL));
	CHECK_FALSE(property_list_has(props, "ldr_scale", Variant::INT));
}

TEST_CASE("[OIT] Compositor effect defaults match design spec") {
	Ref<CompositorEffectOIT> eff;
	eff.instantiate();
	CHECK(eff->get_ldr_scale() == doctest::Approx(0.5f));
	CHECK(eff->get_msaa_enabled() == false);
	CHECK(eff->get_rt_format() == CompositorEffectOIT::RT_FORMAT_HDR);
	CHECK(eff->get_average_depth_complexity() == doctest::Approx(4.0f));
	CHECK(eff->get_debug_overlay_enabled() == false);
}

TEST_CASE("[OIT] Compositor effect enum values are stable") {
	CHECK(CompositorEffectOIT::RT_FORMAT_LDR == 0);
	CHECK(CompositorEffectOIT::RT_FORMAT_HDR == 1);
}

TEST_CASE("[OIT] Compositor effect signal is registered") {
	List<MethodInfo> signals;
	ClassDB::get_signal_list("OrderIndependentTransparencyCompositorEffect", &signals, true);
	CHECK(signal_list_has(signals, "order_independent_transparency_stats", 1));
	CHECK_FALSE(signal_list_has(signals, "oit_stats", 1));
}

TEST_CASE("[OIT] Compositor is willing to hold our subclass in its typed effect array") {
	Ref<Compositor> comp;
	comp.instantiate();
	Ref<CompositorEffectOIT> eff;
	eff.instantiate();
	TypedArray<CompositorEffect> effects;
	effects.push_back(eff);
	comp->set_compositor_effects(effects);
	TypedArray<CompositorEffect> back = comp->get_compositor_effects();
	REQUIRE(back.size() == 1);
	Ref<CompositorEffect> ref_back = back[0];
	CHECK(Object::cast_to<CompositorEffectOIT>(ref_back.ptr()) != nullptr);
}

TEST_CASE("[OIT] Geometry relay class registered with correct parent") {
	CHECK(ClassDB::class_exists("OrderIndependentTransparencyGeometryRelay"));
	CHECK(ClassDB::get_parent_class("OrderIndependentTransparencyGeometryRelay") == StringName("Node3D"));
	CHECK(ClassDB::is_parent_class("OrderIndependentTransparencyGeometryRelay", "Node"));
	CHECK_FALSE(ClassDB::is_parent_class("OrderIndependentTransparencyGeometryRelay", "Resource"));
}

TEST_CASE("[OIT] Geometry relay property surface is exact") {
	List<PropertyInfo> props;
	ClassDB::get_property_list("OrderIndependentTransparencyGeometryRelay", &props, true);
	CHECK(property_list_has(props, "compositor_effect_path", Variant::NODE_PATH));
	CHECK(property_list_has(props, "auto_repack", Variant::BOOL));
	CHECK(property_list_has(props, "mesh_paths", Variant::PACKED_STRING_ARRAY));
}

TEST_CASE("[OIT] Geometry relay signal is registered with a single Array param") {
	List<MethodInfo> signals;
	ClassDB::get_signal_list("OrderIndependentTransparencyGeometryRelay", &signals, true);
	CHECK(signal_list_has(signals, "transparency_batches_ready", 1));
	CHECK_FALSE(signal_list_has(signals, "transparency_batches_ready", 0));
	CHECK_FALSE(signal_list_has(signals, "transparency_batches_ready", 2));
	CHECK_FALSE(signal_list_has(signals, "oit_batches_ready", 1));
}

TEST_CASE("[OIT] ProjectSettings register under rendering/order_independent_transparency/") {
	CHECK(ProjectSettings::get_singleton()->has_setting("rendering/order_independent_transparency/enabled"));
	CHECK(ProjectSettings::get_singleton()->has_setting("rendering/order_independent_transparency/ldr_scale"));
	CHECK(ProjectSettings::get_singleton()->has_setting("rendering/order_independent_transparency/rt_format"));
	CHECK(ProjectSettings::get_singleton()->has_setting("rendering/order_independent_transparency/average_depth_complexity"));
	CHECK(ProjectSettings::get_singleton()->has_setting("rendering/order_independent_transparency/msaa_enabled"));
	CHECK(ProjectSettings::get_singleton()->has_setting("rendering/order_independent_transparency/debug_overlay"));
	CHECK_FALSE(ProjectSettings::get_singleton()->has_setting("rendering/oit/enabled"));
}

TEST_CASE("[OIT] Internal helper classes register") {
	CHECK(ClassDB::class_exists("OrderIndependentTransparencyRenderer"));
	CHECK(ClassDB::class_exists("OrderIndependentTransparencyBufferPool"));
	CHECK(ClassDB::class_exists("OrderIndependentTransparencyDebugOverlay"));
}

// -------- property tests via witness-cpp --------

TEST_CASE("[OIT][Property] set_ldr_scale is idempotent over gen_float") {
	::witness::Generator<float> gen = &::witness::gen_float;
	std::function<bool(const float &)> pred = [](const float &x) {
		Ref<CompositorEffectOIT> e;
		e.instantiate();
		e->set_ldr_scale(x);
		return e->get_ldr_scale() == x;
	};
	::witness::Trial t = ::witness::resolve<float>("ldr_scale round-trip", gen, pred);
	INFO(t.message);
	CHECK(t.outcome == ::witness::Outcome::PROVABLY_NONE);
}

TEST_CASE("[OIT][Property] set_average_depth_complexity is idempotent over gen_float") {
	::witness::Generator<float> gen = &::witness::gen_float;
	std::function<bool(const float &)> pred = [](const float &x) {
		Ref<CompositorEffectOIT> e;
		e.instantiate();
		e->set_average_depth_complexity(x);
		return e->get_average_depth_complexity() == x;
	};
	::witness::Trial t = ::witness::resolve<float>("average_depth_complexity round-trip", gen, pred);
	INFO(t.message);
	CHECK(t.outcome == ::witness::Outcome::PROVABLY_NONE);
}

TEST_CASE("[OIT][Property] set_msaa_enabled is idempotent over gen_bool") {
	::witness::Generator<bool> gen = &::witness::gen_bool;
	std::function<bool(const bool &)> pred = [](const bool &x) {
		Ref<CompositorEffectOIT> e;
		e.instantiate();
		e->set_msaa_enabled(x);
		return e->get_msaa_enabled() == x;
	};
	::witness::Trial t = ::witness::resolve<bool>("msaa_enabled round-trip", gen, pred);
	INFO(t.message);
	CHECK(t.outcome == ::witness::Outcome::PROVABLY_NONE);
}

TEST_CASE("[OIT][Property] set_rt_format round-trips over {LDR, HDR}") {
	std::vector<CompositorEffectOIT::RenderTargetFormat> values = {
		CompositorEffectOIT::RT_FORMAT_LDR,
		CompositorEffectOIT::RT_FORMAT_HDR,
	};
	::witness::Generator<CompositorEffectOIT::RenderTargetFormat> gen = ::witness::gen_element(values);
	std::function<bool(const CompositorEffectOIT::RenderTargetFormat &)> pred =
			[](const CompositorEffectOIT::RenderTargetFormat &fmt) {
				Ref<CompositorEffectOIT> e;
				e.instantiate();
				e->set_rt_format(fmt);
				return e->get_rt_format() == fmt;
			};
	::witness::Trial t = ::witness::resolve<CompositorEffectOIT::RenderTargetFormat>(
			"rt_format round-trip", gen, pred);
	INFO(t.message);
	CHECK(t.outcome == ::witness::Outcome::PROVABLY_NONE);
}

TEST_CASE("[OIT][Property] relay set_auto_repack is idempotent over gen_bool") {
	::witness::Generator<bool> gen = &::witness::gen_bool;
	std::function<bool(const bool &)> pred = [](const bool &x) {
		RelayOIT *r = memnew(RelayOIT);
		r->set_auto_repack(x);
		const bool ok = r->get_auto_repack() == x;
		memdelete(r);
		return ok;
	};
	::witness::Trial t = ::witness::resolve<bool>("auto_repack round-trip", gen, pred);
	INFO(t.message);
	CHECK(t.outcome == ::witness::Outcome::PROVABLY_NONE);
}

// -------- falsifiability twins: prove the harness catches broken worlds --------

TEST_CASE("[OIT][Falsifiability] a broken ldr_scale setter would be caught") {
	// Plant a lie ("setting x always leaves the value at 0"). If the harness
	// were unable to detect a real regression, this Trial would return
	// PROVABLY_NONE and we would ship broken code silently. The property
	// tests above only earn trust if this one FINDS a counterexample.
	::witness::Generator<float> gen = ::witness::gen_double_range(-100.0, 100.0);
	// gen_double_range returns Generator<double>; wrap it to a float generator.
	::witness::Generator<float> gen_f =
			[](::witness::RNG &rng, const ::witness::Level &lvl) -> float {
		return static_cast<float>(rng.float_range(-100.0, 100.0));
	};
	std::function<bool(const float &)> pred = [](const float &x) {
		Ref<CompositorEffectOIT> e;
		e.instantiate();
		e->set_ldr_scale(x);
		// Deliberately wrong: claim get always returns 0.
		return e->get_ldr_scale() == 0.0f;
	};
	::witness::Trial t = ::witness::resolve<float>("planted-false: setter drops to 0", gen_f, pred);
	INFO(t.message);
	CHECK(t.outcome == ::witness::Outcome::FOUND);
	(void)gen; // silence unused warning; kept to show the generator combinator would also work.
}

TEST_CASE("[OIT][Falsifiability] a broken rt_format enum would be caught") {
	std::vector<CompositorEffectOIT::RenderTargetFormat> values = {
		CompositorEffectOIT::RT_FORMAT_LDR,
		CompositorEffectOIT::RT_FORMAT_HDR,
	};
	::witness::Generator<CompositorEffectOIT::RenderTargetFormat> gen = ::witness::gen_element(values);
	std::function<bool(const CompositorEffectOIT::RenderTargetFormat &)> pred =
			[](const CompositorEffectOIT::RenderTargetFormat &fmt) {
				Ref<CompositorEffectOIT> e;
				e.instantiate();
				e->set_rt_format(fmt);
				// Deliberately wrong: claim get always returns LDR.
				return e->get_rt_format() == CompositorEffectOIT::RT_FORMAT_LDR;
			};
	::witness::Trial t = ::witness::resolve<CompositorEffectOIT::RenderTargetFormat>(
			"planted-false: rt_format always LDR", gen, pred);
	INFO(t.message);
	// A random draw of gen_element picks HDR with p=1/2, so a full ladder
	// virtually always FINDs. If the harness ever stopped generating both
	// values, this would fail loudly.
	CHECK(t.outcome == ::witness::Outcome::FOUND);
}

TEST_CASE("[OIT][Falsifiability] planted-false relay flag is caught") {
	::witness::Generator<bool> gen = &::witness::gen_bool;
	std::function<bool(const bool &)> pred = [](const bool &x) {
		RelayOIT *r = memnew(RelayOIT);
		r->set_auto_repack(x);
		// Deliberately wrong: claim the flag flips regardless of input.
		const bool wrong = r->get_auto_repack() == !x;
		memdelete(r);
		return wrong;
	};
	::witness::Trial t = ::witness::resolve<bool>("planted-false: relay flag flips", gen, pred);
	INFO(t.message);
	CHECK(t.outcome == ::witness::Outcome::FOUND);
}

} // namespace TestOrderIndependentTransparency
