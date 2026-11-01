/**************************************************************************/
/*  csg.cpp                                                               */
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

#include "csg.h"

// CSGBrush

void CSGBrush::build_from_faces(const Vector<Vector3> &p_vertices, const Vector<Vector2> &p_uvs, const Vector<bool> &p_smooth, const Vector<Ref<Material>> &p_materials, const Vector<bool> &p_flip_faces) {
	faces.clear();

	int vc = p_vertices.size();

	ERR_FAIL_COND((vc % 3) != 0);

	const Vector3 *rv = p_vertices.ptr();
	int uvc = p_uvs.size();
	const Vector2 *ruv = p_uvs.ptr();
	int sc = p_smooth.size();
	const bool *rs = p_smooth.ptr();
	int mc = p_materials.size();
	const Ref<Material> *rm = p_materials.ptr();
	int ic = p_flip_faces.size();
	const bool *ri = p_flip_faces.ptr();

	HashMap<Ref<Material>, int> material_map;

	faces.resize(p_vertices.size() / 3);

	for (int i = 0; i < faces.size(); i++) {
		Face &f = faces.write[i];
		f.vertices[0] = rv[i * 3 + 0];
		f.vertices[1] = rv[i * 3 + 1];
		f.vertices[2] = rv[i * 3 + 2];

		if (uvc == vc) {
			f.uvs[0] = ruv[i * 3 + 0];
			f.uvs[1] = ruv[i * 3 + 1];
			f.uvs[2] = ruv[i * 3 + 2];
		}

		if (sc == vc / 3) {
			f.smooth = rs[i];
		} else {
			f.smooth = false;
		}

		if (ic == vc / 3) {
			f.invert = ri[i];
		} else {
			f.invert = false;
		}

		if (mc == vc / 3) {
			Ref<Material> mat = rm[i];
			if (mat.is_valid()) {
				HashMap<Ref<Material>, int>::ConstIterator E = material_map.find(mat);

				if (E) {
					f.material = E->value;
				} else {
					f.material = material_map.size();
					material_map[mat] = f.material;
				}

			} else {
				f.material = -1;
			}
		}
	}

	materials.resize(material_map.size());
	for (const KeyValue<Ref<Material>, int> &E : material_map) {
		materials.write[E.value] = E.key;
	}

	_regen_face_aabbs();
}

void CSGBrush::copy_from(const CSGBrush &p_brush, const Transform3D &p_xform) {
	faces = p_brush.faces;
	materials = p_brush.materials;
	has_uv2 = p_brush.has_uv2;
	has_colors = p_brush.has_colors;
	skin_bones = p_brush.skin_bones;
	skin_weights = p_brush.skin_weights;

	for (int i = 0; i < faces.size(); i++) {
		for (int j = 0; j < 3; j++) {
			faces.write[i].vertices[j] = p_xform.xform(p_brush.faces[i].vertices[j]);
		}
	}

	_regen_face_aabbs();
}

void CSGBrush::set_attributes(const Vector<Vector2> &p_uv2s, const Vector<Color> &p_colors, const Vector<int> &p_bones, const Vector<float> &p_weights, int p_influences) {
	const int corners = faces.size() * 3;

	has_uv2 = p_uv2s.size() == corners;
	if (has_uv2) {
		for (int i = 0; i < faces.size(); i++) {
			for (int j = 0; j < 3; j++) {
				faces.write[i].uv2s[j] = p_uv2s[i * 3 + j];
			}
		}
	}

	has_colors = p_colors.size() == corners;
	if (has_colors) {
		for (int i = 0; i < faces.size(); i++) {
			for (int j = 0; j < 3; j++) {
				faces.write[i].colors[j] = p_colors[i * 3 + j];
			}
		}
	}

	skin_bones.clear();
	skin_weights.clear();
	if (p_influences <= 0 || p_bones.size() != corners * p_influences || p_weights.size() != corners * p_influences) {
		return;
	}

	// One channel per bone that carries weight anywhere on the brush, in bone order.
	Vector<int> used;
	for (int i = 0; i < p_bones.size(); i++) {
		if (p_weights[i] > 0.0f && !used.has(p_bones[i])) {
			used.push_back(p_bones[i]);
		}
	}
	used.sort();
	if (used.is_empty()) {
		return;
	}
	ERR_FAIL_COND_MSG(used.size() > MAX_SKIN_CHANNELS, vformat("CSG carries skin weights for at most %d bones, this mesh uses %d. Merge its bones into fewer groups first.", MAX_SKIN_CHANNELS, used.size()));

	const int channels = used.size();
	skin_bones = used;
	skin_weights.resize(corners * channels);
	skin_weights.fill(0.0f);
	float *w = skin_weights.ptrw();
	for (int c = 0; c < corners; c++) {
		for (int k = 0; k < p_influences; k++) {
			const float weight = p_weights[c * p_influences + k];
			if (weight > 0.0f) {
				w[c * channels + used.find(p_bones[c * p_influences + k])] += weight;
			}
		}
	}
}
