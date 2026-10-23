# Procedural City Generator — portable Windows Release

Extract the entire ZIP to a normal folder. Keep the EXE, DLLs and assets folder together.
Double-click ProceduralCityGenerator.exe. No compiler, CMake, Python or MSYS2 installation is required.

Requirements: 64-bit Windows 10/11 (tested on the current Windows 11 machine), a GPU/driver supporting desktop OpenGL 2.1 compatibility rendering, and a desktop display of at least 1000 x 700 pixels. Install the GPU manufacturer's driver if OpenGL initialization fails. Windows supplies WIC, Universal CRT and system DLLs; these are not bundled. Older Windows, Windows ARM emulation, virtual machines and remote desktop are untested.

From PowerShell in this folder:

```powershell
.\ProceduralCityGenerator.exe --demo
.\ProceduralCityGenerator.exe --seed 42
.\ProceduralCityGenerator.exe --demo --smoke-test
```

The smoke test creates a hidden OpenGL window, exercises existing keyboard/mouse/rendering/resize checks and exits. Success prints "Smoke test passed" and "Visual assets: 19/19 loaded". A normal run opens the graphical application; Escape exits.

Controls: R generate, O optimize selected candidate, B before/after, P next incident, G graph overlay; arrow keys edit pending densities. Existing UI controls are unchanged.

Built with GCC 14.1.0 (MSYS2 UCRT64), C++17, CMake Release (-O3 -DNDEBUG). GLFW, GLAD and Dear ImGui are statically linked. Required MinGW runtime DLLs are included beside the executable; do not replace them with unrelated versions. See licenses/ for dependency notices and assets/README.md for artwork provenance. The supplied artwork's redistribution rights have not been independently verified.

Assets are resolved relative to the executable on Windows, including when launched from another working directory. No installation or administrator rights are needed. A console window is expected. This package was tested on one machine, not a clean Windows installation; graphics-driver compatibility remains machine-dependent.
