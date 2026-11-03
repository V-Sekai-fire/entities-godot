extends SceneTree
# OIT defect tests. OIT_TEST = alpha | skin | stack | shimmer, OIT_SCALE multiplies every distance (1 = hair on a character, 1e9 = solar system).
# Every pixel of the render is compared with the exact depth-ordered over-blend of the layers (rays against planes, circles for clipped layers), linear space, black background.
var S := 1.0
var edge_px := 1.0   # pixels around a coverage edge to skip; the froxel tile is 6 px, so a tile-sized skip hides the tile edge
var cam: Camera3D
var layers: Array = []   # {mi, color, radius (0 = unclipped)}
func mat(c: Color) -> StandardMaterial3D:
	var m := StandardMaterial3D.new()
	m.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	m.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	m.cull_mode = BaseMaterial3D.CULL_DISABLED
	m.albedo_color = c
	return m
func clipped_mat(c: Color) -> ShaderMaterial:
	var sh := Shader.new()
	sh.code = "shader_type spatial;\nrender_mode unshaded, cull_disabled, blend_mix;\nuniform vec4 col;\nvoid fragment() { vec2 p = UV - vec2(0.5); if (dot(p, p) > 0.25) { discard; } ALBEDO = col.rgb; ALPHA = col.a; }\n"
	var m := ShaderMaterial.new()
	m.shader = sh
	m.set_shader_parameter("col", c)
	return m
func add_quad(pos: Vector3, yaw: float, c: Color, size: float, clip: bool) -> MeshInstance3D:
	var q := QuadMesh.new()
	q.size = Vector2(size, size)
	var mi := MeshInstance3D.new()
	mi.mesh = q
	mi.material_override = clipped_mat(c) if clip else mat(c)
	mi.position = pos
	mi.rotation.y = deg_to_rad(yaw)
	root.add_child(mi)
	layers.append({"mi": mi, "color": c, "radius": size * 0.5 if clip else 0.0})
	return mi
func expected(px: Vector2) -> Color:
	var o := cam.project_ray_origin(px)
	var d := cam.project_ray_normal(px)
	var hits := []
	for l in layers:
		var mi: MeshInstance3D = l["mi"]
		var n: Vector3 = mi.global_transform.basis.z.normalized()
		var h: Variant = Plane(n, n.dot(mi.global_position)).intersects_ray(o, d)
		if h == null:
			continue
		var local: Vector3 = mi.global_transform.affine_inverse() * (h as Vector3)
		var half: float = (mi.mesh as QuadMesh).size.x * 0.5
		if absf(local.x) > half or absf(local.y) > half:
			continue
		if l["radius"] > 0.0 and Vector2(local.x, local.y).length() > l["radius"]:
			continue
		hits.append([o.distance_to(h), l["color"], Vector2(local.x, local.y).length() / maxf(l["radius"], 1e-9) if l["radius"] > 0.0 else 0.0])
	hits.sort_custom(func(a, b): return a[0] < b[0])
	var acc := Color(0, 0, 0)
	var trans := 1.0
	for h in hits:
		var c: Color = h[1]
		acc.r += c.r * c.a * trans
		acc.g += c.g * c.a * trans
		acc.b += c.b * c.a * trans
		trans *= 1.0 - c.a
	return Color(acc.r, acc.g, acc.b).linear_to_srgb()
func compare(label: String) -> void:
	print("CENTRE got ", root.get_viewport().get_texture().get_image().get_pixel(64, 64), " want ", expected(Vector2(64.5, 64.5)))
	var img := root.get_viewport().get_texture().get_image()
	var w := img.get_width()
	var h := img.get_height()
	var worst := 0.0
	var bad := 0
	var bx0 := 999; var bx1 := -1; var by0 := 999; var by1 := -1
	var n := 0
	for y in range(2, h - 2, 1):
		for x in range(2, w - 2, 1):
			var want := expected(Vector2(x + 0.5, y + 0.5))
			# a pixel next to a coverage edge is a rasterisation tie, not an OIT error: skip it
			var edge := false
			for off in [Vector2(1, 0), Vector2(-1, 0), Vector2(0, 1), Vector2(0, -1)]:
				off *= edge_px
				var nb := expected(Vector2(x + 0.5, y + 0.5) + off)
				if maxf(maxf(absf(nb.r - want.r), absf(nb.g - want.g)), absf(nb.b - want.b)) * 255.0 > 8.0:
					edge = true
			if edge:
				continue
			var got := img.get_pixel(x, y)
			var e := maxf(maxf(absf(got.r - want.r), absf(got.g - want.g)), absf(got.b - want.b)) * 255.0
			worst = maxf(worst, e)
			n += 1
			if e > 8.0:
				bad += 1
				if bad < 12: print("BADPX ", x, ",", y, " got ", got, " want ", want)
				bx0 = mini(bx0, x); bx1 = maxi(bx1, x); by0 = mini(by0, y); by1 = maxi(by1, y)
	print("BAD box x ", bx0, "-", bx1, " y ", by0, "-", by1)
	print("RESULT %s: worst pixel error %.1f/255, %d of %d pixels over 8/255 -> %s" % [label, worst, bad, n, "PASS" if bad * 200 < n else "FAIL"])
func frames(n: int) -> void:
	for i in n:
		await process_frame
func _init() -> void:
	root.size = Vector2i(128, 128)
	S = float(OS.get_environment("OIT_SCALE")) if OS.get_environment("OIT_SCALE") != "" else 1.0
	var env := Environment.new()
	env.background_mode = Environment.BG_COLOR
	env.background_color = Color(0, 0, 0)
	env.tonemap_mode = Environment.TONE_MAPPER_LINEAR
	cam = Camera3D.new()
	cam.environment = env
	cam.fov = 30.0
	cam.near = 0.1 * S
	cam.far = 30.0 * S
	root.add_child(cam)
	await frames(2)
	var R := Color(1, 0, 0, 0.5)
	var B := Color(0, 0, 1, 0.5)
	var test := OS.get_environment("OIT_TEST")
	if test == "alpha":
		# two circular discard-clipped layers crossing at 35 degrees, 2 m away, 2 m wide
		add_quad(Vector3(0, 0, -2.5 * S), 35, R, 2.0 * S, true)
		add_quad(Vector3(0, 0, -2.5 * S), -35, B, 2.0 * S, true)
		await frames(6)
		compare("alpha-clipped crossing circles")
	elif test == "stack":
		# eight hair-strand layers 1 mm apart (at scale 1) at 2 m, alternating colours, alpha 0.3, slightly tilted
		var cols := [Color(1, 0, 0, 0.3), Color(0, 1, 0, 0.3), Color(0, 0, 1, 0.3), Color(1, 1, 0, 0.3)]
		for i in 8:
			add_quad(Vector3(0, 0, -(2.0 + 0.001 * i) * S), 12.0, cols[i % 4], 0.3 * S, false)
		await frames(6)
		compare("hair stack, 8 layers 1 mm apart (relative)")
	elif test == "behind":
		# the pair 2 mm apart at 2 m, and a big window 25 m behind it filling the view
		add_quad(Vector3(0, 0, -2.0 * S), 0, R, 1.0 * S, false)
		add_quad(Vector3(0, 0, -(2.0 + 0.002) * S), 0, B, 1.0 * S, false)
		edge_px = 7.0
		add_quad(Vector3(0, 0, -25.0 * S), 0, Color(0, 1, 0, 0.5), 40.0 * S, false)
		await frames(6)
		compare("pair 2 mm apart in front of a window 25 m behind")
	elif test == "skin":
		# two layers 2 mm apart, skinned: the bone moves them 1.5 m away from the rest bounds the instance carries
		var sk := Skeleton3D.new()
		root.add_child(sk)
		sk.add_bone("b")
		sk.set_bone_rest(0, Transform3D.IDENTITY)
		sk.set_bone_pose_position(0, Vector3(0, 0, -1.5 * S))
		var shared_skin := sk.create_skin_from_rest_transforms()
		for i in 2:
			var arr := []
			arr.resize(Mesh.ARRAY_MAX)
			var z := -(0.5 + 0.002 * i) * S
			var hs := 4.0 * S
			arr[Mesh.ARRAY_VERTEX] = PackedVector3Array([Vector3(-hs, -hs, z), Vector3(hs, -hs, z), Vector3(hs, hs, z), Vector3(-hs, hs, z)])
			arr[Mesh.ARRAY_BONES] = PackedInt32Array([0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0])
			arr[Mesh.ARRAY_WEIGHTS] = PackedFloat32Array([1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0])
			arr[Mesh.ARRAY_INDEX] = PackedInt32Array([0, 1, 2, 0, 2, 3])
			var am := ArrayMesh.new()
			am.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, arr)
			var mi := MeshInstance3D.new()
			mi.mesh = am
			mi.material_override = mat(R if i == 0 else B)
			mi.mesh.set_meta("i", i)
			sk.add_child(mi)
			if OS.get_environment("OIT_AABB") != "": mi.custom_aabb = AABB(Vector3(-5, -5, -5) * S, Vector3(10, 10, 10) * S)
			mi.skin = shared_skin
			mi.skeleton = NodePath("..")
			# the layers sit at z = -(0.5 + 0.002 i) before skinning and -(2.0 + 0.002 i) after; the expected image uses the skinned planes
			var proxy := MeshInstance3D.new()
			var q := QuadMesh.new()
			q.size = Vector2(hs * 2.0, hs * 2.0)
			proxy.mesh = q
			proxy.position = Vector3(0, 0, -(2.0 + 0.002 * i) * S)
			proxy.visible = false
			root.add_child(proxy)
			layers.append({"mi": proxy, "color": R if i == 0 else B, "radius": 0.0})
		await frames(8)
		compare("skinned layers 2 mm apart, moved far outside their rest bounds")
	elif test == "shimmer":
		# a static pair 2 mm apart at 2 m, and a translucent quad that sweeps in depth from 1 to 40 times as far, changing the range every frame
		add_quad(Vector3(0, 0, -2.0 * S), 0, R, 1.0 * S, false)
		add_quad(Vector3(0, 0, -(2.0 + 0.002) * S), 0, B, 1.0 * S, false)
		var traveller := add_quad(Vector3(2.0 * S, 0, -1.0 * S), 0, Color(0, 1, 0, 0.5), 0.5 * S, false)
		layers.erase(layers.back())   # the traveller is off to the side and not part of the expected centre pixel
		await frames(4)
		var lo := 1000.0
		var hi := -1000.0
		var want := expected(Vector2(64, 64)).g
		for f in 40:
			traveller.position.z = -(1.0 + float(f)) * S
			await frames(2)
			var px := root.get_viewport().get_texture().get_image().get_pixel(64, 64)
			print("SWEEP depth ", f + 1, " red ", snappedf(px.r, 0.001))
			lo = minf(lo, px.r)
			hi = maxf(hi, px.r)
		print("RESULT range-sweep shimmer of a static pair: centre red varied %.1f/255 over 40 frames (spread of the range 1x to 40x) -> %s" % [(hi - lo) * 255.0, "PASS" if (hi - lo) * 255.0 < 4.0 else "FAIL"])
	quit(0)
