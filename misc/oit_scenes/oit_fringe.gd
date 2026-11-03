extends SceneTree
# Black-line check: a flat card at 2 m, yaw YAW degrees, size factor SZ, NOTEX=1 for a plain white card. Prints the dark pixel columns (DARKCOLS must be empty) and the darkest and mean luma; run it against a project with rendering/oit/enabled on and off, the two must match. Before the inverse-depth lookup a card at yaw 20 had a two-pixel black column at a tile edge.
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
	var img := Image.create(64, 64, true, Image.FORMAT_RGBA8)
	for y in 64:
		for x in 64:
			var d := Vector2(x - 31.5, y - 31.5).length() / 28.0
			var a := clampf(1.0 - d, 0.0, 1.0) * 3.0
			a = clampf(a, 0.0, 1.0)
			# colour: warm yellow inside, black where alpha is 0
			var c := Color(0.9, 0.7, 0.3) * (1.0 if a > 0.0 else 0.0)
			img.set_pixel(x, y, Color(c.r, c.g, c.b, a))
	img.generate_mipmaps()
	var tex := ImageTexture.create_from_image(img)
	var m := StandardMaterial3D.new()
	m.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	m.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	if OS.get_environment("NOTEX") == "": m.albedo_texture = tex
	m.cull_mode = BaseMaterial3D.CULL_DISABLED
	var q := QuadMesh.new()
	q.size = Vector2(0.8, 0.8) * float(OS.get_environment("SZ"))
	var mi := MeshInstance3D.new()
	mi.mesh = q
	mi.material_override = m
	mi.position = Vector3(0, 0, -2.0)
	mi.rotation.y = deg_to_rad(float(OS.get_environment("YAW")))
	root.add_child(mi)
	for i in 8:
		await process_frame
	var out := root.get_viewport().get_texture().get_image()
	var darkest := 1.0
	var sum := 0.0
	var n := 0
	for y in 128:
		for x in 128:
			var p := out.get_pixel(x, y)
			var l := (p.r + p.g + p.b) / 3.0
			# only pixels near the card (not pure background)
			if absf(l - 0.9) > 0.01 or p.b < 0.89:
				darkest = minf(darkest, l)
				sum += l
				n += 1
	var cols := []
	for x in 128:
		var col_dark := 0
		for y in 128:
			var pp := out.get_pixel(x, y)
			if (pp.r + pp.g + pp.b) / 3.0 < 0.2:
				col_dark += 1
		if col_dark > 0:
			cols.append([x, col_dark])
	print("DARKCOLS ", cols)
	print("FRINGE card pixels %d, darkest luma %.3f, mean luma %.3f" % [n, darkest, sum / maxf(n, 1)])
	out.save_png("/tmp/fringe_%s.png" % OS.get_environment("TAG"))
	quit(0)
