# Ray Tracing — "Le Voyage de Chihiro" (Sixième Station)

CPU ray tracer in **C** with **SDL2** output.

## Build / Run

This Makefile tries `sdl2-config`, then `pkg-config` (MSYS2 / Unix-like environment).

### Windows (PowerShell)

```powershell
.\build.ps1
.\run.ps1
```

```bash
make
make run
```

## Controls (coming soon)

- `ESC` / window close: quit
- `SPACE`: toggle **auto cinematic** / **interactive**
- Interactive: `W/S` forward/back, `A/D` strafe left/right

## Performance trick

While the camera is moving, rendering uses a lower resolution (pixel blocks) for speed.
