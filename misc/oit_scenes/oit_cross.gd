extends SceneTree
# OIT test 2: two Mix-blended quads (red, blue, alpha 0.5) crossing in an X, so on one side of the
# crossing red is in front and on the other blue is. Sorted blending orders whole objects, so one
# side is always wrong; per-pixel correct compositing gets both. Prints both sides against the
# analytic over-blend (linear, white background, tonemap linear) and the error. MSAA=<0|2|4> env.
var f := 0
const BG := Color(1, 1, 1)


func _initialize() -> void:
	var msaa := int(OS.get_environment("MSAA")) if OS.get_environment("MSAA") != "" else 0
	get_root().msaa_3d = [Viewport.MSAA_DISABLED, Viewport.MSAA_DISABLED, Viewport.MSAA_2X, Viewport.MSAA_2X, Viewport.MSAA_4X][msaa]
	var root := Node3D.new()
	get_root().add_child(root)
	var cam := Camera3D.new()
	cam.position = Vector3(0, 0, 3)
	root.add_child(cam)
	cam.current = true
	var env := WorldEnvironment.new()
	env.environment = Environment.new()
	env.environment.background_mode = Environment.BG_COLOR
	env.environment.background_color = BG
	env.environment.tonemap_mode = Environment.TONE_MAPPER_LINEAR
	root.add_child(env)
	for i in 2:
		var mi := MeshInstance3D.new()
		var q := QuadMesh.new()
		q.size = Vector2(2.0, 1.0)
		mi.mesh = q
		var m := StandardMaterial3D.new()
		m.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
		m.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
		m.cull_mode = BaseMaterial3D.CULL_DISABLED
		m.albedo_color = Color(1, 0, 0, 0.5) if i == 0 else Color(0, 0, 1, 0.5)
		mi.material_override = m
		mi.rotation.y = deg_to_rad(35.0) * (1.0 if i == 0 else -1.0)
		root.add_child(mi)
	print("OITX renderer ", RenderingServer.get_current_rendering_method(), " oit ", ProjectSettings.get_setting("rendering/oit/enabled", "n/a"), " msaa ", msaa)


static func over(front: Color, back: Color) -> Color:
	var under := Color(back.r * back.a + BG.r * (1.0 - back.a), back.g * back.a + BG.g * (1.0 - back.a), back.b * back.a + BG.b * (1.0 - back.a))
	return Color(front.r * front.a + under.r * (1.0 - front.a), front.g * front.a + under.g * (1.0 - front.a), front.b * front.a + under.b * (1.0 - front.a))


func _process(_d: float) -> bool:
	f += 1
	if f < 8:
		return false
	var img := get_root().get_texture().get_image()
	var w := img.get_width()
	var h := img.get_height()
	var red := Color(1, 0, 0, 0.5)
	var blue := Color(0, 0, 1, 0.5)
	# red quad turns +35 deg about y: its +x end goes away from the camera (-z), so red is in front at x < 0
	var worst := 0.0
	# the drawn area (the window may render into part of the texture): bbox of non-background
	var x0 := w
	var x1 := 0
	var y0 := h
	var y1 := 0
	for y in h:
		for x in w:
			if img.get_pixel(x, y).r + img.get_pixel(x, y).g + img.get_pixel(x, y).b < 2.95:
				x0 = mini(x0, x)
				x1 = maxi(x1, x)
				y0 = mini(y0, y)
				y1 = maxi(y1, y)
	var cx := (x0 + x1) / 2
	var cy := (y0 + y1) / 2
	var dx := (x1 - x0) / 6
	for side in [-1, 1]:
		var px := img.get_pixel(cx + side * dx, cy).srgb_to_linear()
		var want := over(red, blue) if side < 0 else over(blue, red)
		var e := maxf(absf(px.r - want.r), maxf(absf(px.g - want.g), absf(px.b - want.b)))
		worst = maxf(worst, e)
		print("OITX side %+d got %s want %s err %.4f" % [side, px, want, e])
	print("OITX worst err %.4f" % worst)
	return true
