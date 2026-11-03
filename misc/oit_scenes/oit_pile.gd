extends SceneTree
# Many overlapping cards at alpha 1.0 (an opaque-looking shader that writes ALPHA, like the viewer's checker): count the black pixels. Colours are bright, so any pixel under luma 0.15 is an artifact.
func _init() -> void:
	root.size = Vector2i(128, 128)
	var env := Environment.new()
	env.background_mode = Environment.BG_COLOR
	env.background_color = Color(0.9, 0.9, 0.9)
	env.tonemap_mode = Environment.TONE_MAPPER_LINEAR
	var cam := Camera3D.new()
	cam.environment = env
	cam.fov = 30.0
	root.add_child(cam)
	var rng := RandomNumberGenerator.new()
	var cards: Array = []
	var cols: Array = []
	rng.seed = 7
	var n_cards := int(OS.get_environment("CARDS")) if OS.get_environment("CARDS") != "" else 40
	var alpha := float(OS.get_environment("ALPHA")) if OS.get_environment("ALPHA") != "" else 1.0
	for i in n_cards:
		var m := StandardMaterial3D.new()
		m.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
		m.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
		m.cull_mode = BaseMaterial3D.CULL_DISABLED
		m.albedo_color = Color(rng.randf_range(0.5, 1.0), rng.randf_range(0.5, 1.0), rng.randf_range(0.5, 1.0), alpha)
		var q := QuadMesh.new()
		q.size = Vector2(0.5, 0.5)
		var mi := MeshInstance3D.new()
		mi.mesh = q
		mi.material_override = m
		mi.position = Vector3(rng.randf_range(-0.2, 0.2), rng.randf_range(-0.2, 0.2), -2.0 - rng.randf_range(0.0, 0.3))
		mi.rotation = Vector3(rng.randf_range(-0.6, 0.6), rng.randf_range(-0.6, 0.6), rng.randf_range(0.0, 3.0))
		root.add_child(mi)
		cards.append(mi)
		cols.append(m.albedo_color)
	for i in 8:
		await process_frame
	var out := root.get_viewport().get_texture().get_image()
	var black := 0
	var covered := 0
	for y in 128:
		for x in 128:
			var p := out.get_pixel(x, y)
			if absf(p.r - 0.9) > 0.01 or absf(p.g - 0.9) > 0.01:
				covered += 1
			if (p.r + p.g + p.b) / 3.0 < 0.15:
				black += 1
	# exact answer for alpha 1: the nearest card along each pixel ray
	var wrong := 0
	for y in range(2, 126):
		for x in range(2, 126):
			var o := cam.project_ray_origin(Vector2(x + 0.5, y + 0.5))
			var d := cam.project_ray_normal(Vector2(x + 0.5, y + 0.5))
			var best := 1.0e9
			var want := Color(0.9, 0.9, 0.9)
			for ci in cards.size():
				var c: MeshInstance3D = cards[ci]
				var n := c.global_transform.basis.z.normalized()
				var h: Variant = Plane(n, n.dot(c.global_position)).intersects_ray(o, d)
				if h == null:
					continue
				var lp: Vector3 = c.global_transform.affine_inverse() * (h as Vector3)
				if absf(lp.x) > 0.25 or absf(lp.y) > 0.25:
					continue
				var dist := o.distance_to(h)
				if dist < best:
					best = dist
					want = cols[ci] as Color
			var got := out.get_pixel(x, y)
			if maxf(maxf(absf(got.r - want.r), absf(got.g - want.g)), absf(got.b - want.b)) * 255.0 > 8.0:
				wrong += 1
	print("EXACT wrong pixels vs nearest-layer (edges included): ", wrong)
	print("PILE cards %d alpha %.2f: covered %d px, black %d px" % [n_cards, alpha, covered, black])
	out.save_png("/tmp/pile_%s.png" % OS.get_environment("TAG"))
	quit(0)
