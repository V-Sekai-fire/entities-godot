# OIT scene checks

Windowed scripts that render on the GPU and print a pass/fail number. Put each in a project whose
project.godot sets rendering/oit/enabled, then run

    godot --path <project> --resolution 320x180 --rendering-method <forward_plus|mobile> --script res://oit_order.gd

- oit_order.gd: three overlapping Mix quads drawn back-to-front, then front-to-back. Prints the
  largest channel difference between the two orders: 0.27 with sorted blending, 0 with OIT.
- oit_cross.gd: two quads crossing in an X, each in front on one side. Prints the worst error against
  the exact over-blend: 0.25 with sorted blending, under 0.005 with OIT. MSAA=4 in the environment
  turns on 4x MSAA.
