#!/usr/bin/env pwsh
param(
	[string]$Out = "raytracer.exe"
)

$ErrorActionPreference = "Stop"

if (-not (Get-Command gcc -ErrorAction SilentlyContinue)) {
	throw "gcc not found. Install MSYS2 MinGW-w64 toolchain."
}
if (-not (Get-Command pkg-config -ErrorAction SilentlyContinue)) {
	throw "pkg-config not found. Install SDL2 pkg-config files (MSYS2: mingw-w64-x86_64-pkgconf + mingw-w64-x86_64-SDL2)."
}

$flags = (pkg-config --cflags --libs sdl2) -split ' '

gcc -std=c11 -O2 -Wall -Wextra -Wpedantic -Iinclude `
	src/main.c `
	src/app.c `
	src/camera.c `
	@flags `
	-o $Out
