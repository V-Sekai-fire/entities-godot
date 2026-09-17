import Oit.Voxelize
import Oit.Integrate

/-!
`lake exe emit_shaders <dir>` writes the AVBOIT Slang sources under
`<dir>/`. The Godot module includes them next to the hand-written
GLSL wrappers, and a build-time hash check in SCsub keeps a
hand-edit of an emitted file from surviving CI.
-/

def emitOne (dir shaderName : String) (src : String) : IO Unit := do
  let path := System.mkFilePath [dir, shaderName]
  IO.println s!"emit: {path}"
  IO.FS.writeFile path src

def main (args : List String) : IO UInt32 := do
  match args with
  | [dir] =>
    IO.FS.createDirAll dir
    emitOne dir "oit_voxelize.slang" (LeanSlang.emit Oit.Voxelize.shader)
    emitOne dir "oit_integrate.slang" (LeanSlang.emit Oit.Integrate.shader)
    return 0
  | _ =>
    IO.eprintln "usage: emit_shaders <output-dir>"
    return 1
