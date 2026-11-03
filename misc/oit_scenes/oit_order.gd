extends SceneTree
# OIT order test: three overlapping Mix-blended quads (red, green, blue, alpha 0.5) at different
# depths, drawn in two opposite orders (render_priority). Prints the center pixel of each order and
# their difference: sorted blending depends on draw order, OIT should not. A second pair moves the
# camera so the depth order flips while the priorities stay, as a control on the depth sort.
#   godot --path . --resolution 320x180 --rendering-method <forward_plus|mobile> --script <this>
var f := 0
var quads: Array = []
var shots: Array = []
var step := 0
const COLS := [Color(1, 0, 0, 0.5), Color(0, 1, 0, 0.5), Color(0, 0, 1, 0.5)]


func _initialize() -> void:
	var root := Node3D.new()
	get_root().add_child(root)
	var cam := Camera3D.new()
	cam.position = Vector3(0, 0, 3)
	root.add_child(cam)
	cam.current = true
	var env := WorldEnvironment.new()
	env.environment = Environment.new()
	env.environment.background_mode = Environment.BG_COLOR
	env.environment.background_color = Color(1, 1, 1)
	env.environment.tonemap_mode = Environment.TONE_MAPPER_LINEAR
	root.add_child(env)
	for i in 3:
		var mi := MeshInstance3D.new()
		var q := QuadMesh.new()
		q.size = Vector2(1.5, 1.5)
		mi.mesh = q
		var m := StandardMaterial3D.new()
		m.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
		m.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
		m.albedo_color = COLS[i]
		mi.material_override = m
		mi.position = Vector3(0, 0, -0.5 * i)
		root.add_child(mi)
		quads.append(mi)
	print("OITTEST renderer ", RenderingServer.get_current_rendering_method(), " oit ", ProjectSettings.get_setting("rendering/oit/enabled", "n/a"))
	_order(true)


## Back-to-front by priority (true) or front-to-back (false): the order sorted blending draws in.
func _order(back_first: bool) -> void:
	for i in 3:
		var p := (2 - i) if back_first else i
		(quads[i].material_override as StandardMaterial3D).render_priority = p - 1


func _process(_d: float) -> bool:
	f += 1
	if f % 6 != 0:
		return false
	var img := get_root().get_texture().get_image()
	shots.append(img.get_pixel(img.get_width() / 2, img.get_height() / 2))
	step += 1
	if step == 1:
		_order(false)
	elif step == 2:
		var a: Color = shots[0]
		var b: Color = shots[1]
		var d := maxf(absf(a.r - b.r), maxf(absf(a.g - b.g), absf(a.b - b.b)))
		print("OITTEST back-first %s front-first %s max diff %.4f" % [a, b, d])
		return true
	return false
