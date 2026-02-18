#!/usr/bin/env pwsh

$ErrorActionPreference = "Stop"

& "$PSScriptRoot\\build.ps1" -Out "raytracer.exe"
& "$PSScriptRoot\\raytracer.exe"
