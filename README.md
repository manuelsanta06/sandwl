# SandWl

Basic wayland compositor built just for learning

Building
---------
To compile the project, simply clone the repository and run `make` :

```bash
git clone https://github.com/manuelsanta06/sandwl.git
cd sandwl
make
```
The compiled binary will be located at `build/compositor`.
or use `sudo make install` to add it directly to '/usr/local/bin/'

Camera controls
---------------
Configure camera controls in `~/.config/sandwl/config.lua`:

```lua
sand.cameraMode.bind("W",sand.cameraMode.forward)
sand.cameraMode.bind("S",sand.cameraMode.backward)
sand.cameraMode.bind("A",sand.cameraMode.left)
sand.cameraMode.bind("D",sand.cameraMode.right)

sand.bind("ALT+SPACE",sand.cameraMode.toggle)

sand.cameraMode.bind("Q",function(pressed)
  if pressed then
    sand.log("Camera action")
  end
end)
```

Camera bindings take priority over normal bindings while camera mode is enabled.
Other keys follow the normal input path. Callbacks receive `true` on press and
`false` on release. Disabling the mode also releases active callbacks and clears
the movement flags; the later physical release is consumed without calling them
again. `sand.cameraMode.enable()` and `.disable()` explicitly select the mode.
The direction functions store input state; connecting it to the 3D camera is a
separate step. The existing `sand.cameras.current()` API remains available.

Run the keyboard and Lua integration tests with `make test`.
