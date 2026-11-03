extends SceneTree
# Two translucent layers (red and blue, alpha 0.5, unlit) 2, 5 and 10 mm apart at five depths over a black background, against the exact depth-ordered over-blend; prints OK or WRONG per case. Put it in a project whose project.godot sets rendering/oit/*.
var cam: Camera3D
func mat(c: Color) -> StandardMaterial3D:
	var m := StandardMaterial3D.new()
	m.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	m.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	m.cull_mode = BaseMaterial3D.CULL_DISABLED
	m.albedo_color = c
	return m
func quad(pos: Vector3, yaw: float, c: Color) -> MeshInstance3D:
	var q := QuadMesh.new()
	q.size = Vector2(8, 8) * S
	var mi := MeshInstance3D.new()
	mi.mesh = q
	mi.material_override = mat(c)
	mi.position = pos
	mi.rotation.y = deg_to_rad(yaw)
	return mi
func plane_of(mi: MeshInstance3D) -> Plane:
	var n: Vector3 = mi.global_transform.basis.z.normalized()
	return Plane(n, n.dot(mi.global_position))
func expected(px: Vector2, layers: Array) -> Color:
	var o := cam.project_ray_origin(px)
	var d := cam.project_ray_normal(px)
	var hits := []
	for mi in layers:
		var h: Variant = plane_of(mi).intersects_ray(o, d)
		if h != null:
			hits.append([o.distance_to(h), (mi.material_override as StandardMaterial3D).albedo_color])
	hits.sort_custom(func(a, b): return a[0] < b[0])   # nearest first
	var acc := Color(0, 0, 0, 0)   # premultiplied, front to back
	var trans := 1.0
	for h in hits:
		var c: Color = h[1]
		acc.r += c.r * c.a * trans
		acc.g += c.g * c.a * trans
		acc.b += c.b * c.a * trans
		trans *= 1.0 - c.a
	return Color(acc.r, acc.g, acc.b).linear_to_srgb()   # background is black
func shot(layers: Array, pts: Array, label: String) -> void:
	for l in get_nodes_in_group("layer"):
		l.queue_free()
	await process_frame
	for l in layers:
		l.add_to_group("layer")
		root.add_child(l)
	for i in 6:
		await process_frame
	var img := root.get_viewport().get_texture().get_image()
	for p in pts:
		var got := img.get_pixelv(p)
		var want := expected(Vector2(p), layers)
		var err := maxf(maxf(absf(got.r - want.r), absf(got.g - want.g)), absf(got.b - want.b)) * 255.0
		print("%s px %s: got (%3d,%3d,%3d) want (%3d,%3d,%3d)  worst channel error %5.1f/255  %s" % [label, str(p), got.r8, got.g8, got.b8, want.r8, want.g8, want.b8, err, "OK" if err < 4.0 else "WRONG"])
var S := 1.0
func _init() -> void:
	S = float(OS.get_environment("OIT_SCALE")) if OS.get_environment("OIT_SCALE") != "" else 1.0   # scale every distance: 1 is hair scale (millimetres at two metres), 1e9 is solar-system scale
	root.size = Vector2i(128, 128)
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
	await process_frame
	var R := Color(1, 0, 0, 0.5)
	var B := Color(0, 0, 1, 0.5)
	var mid := [Vector2i(64, 64)]
	await shot([quad(Vector3(0, 0, -2), 0, R), quad(Vector3(0, 0, -3), 0, B)], mid, "A red front, red drawn first ")
	await shot([quad(Vector3(0, 0, -3), 0, B), quad(Vector3(0, 0, -2), 0, R)], mid, "B red front, blue drawn first")
	await shot([quad(Vector3(0, 0, -2), 0, R), quad(Vector3(0, 0, -3), 0, B)], mid, "A again (repeat)            ")
	await shot([quad(Vector3(0, 0, -3), 0, R), quad(Vector3(0, 0, -2), 0, B)], mid, "C blue front                 ")
	await shot([quad(Vector3(0, 0, -2.5), 35, R), quad(Vector3(0, 0, -2.5), -35, B)], [Vector2i(30, 64), Vector2i(98, 64)], "D crossing red-first        ")
	await shot([quad(Vector3(0, 0, -2.5), -35, B), quad(Vector3(0, 0, -2.5), 35, R)], [Vector2i(30, 64), Vector2i(98, 64)], "D crossing blue-first       ")
	for z0 in [-1.85, -2.0, -2.2, -2.4, -2.55]:
		for dz in [0.002, 0.005, 0.01]:
			await shot([quad(Vector3(0, 0, z0 * S), 0, R), quad(Vector3(0, 0, (z0 - dz) * S), 0, B)], mid, "Z z=%.2f sep %.1f mm" % [z0, dz * 1000.0])
	quit(0)
