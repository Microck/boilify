@echo off
set PLUGIN_DIR=C:\Program Files\Common Files\OFX\Plugins

echo Installing Boilify to %PLUGIN_DIR%

if not exist "%PLUGIN_DIR%" mkdir "%PLUGIN_DIR%"
xcopy /E /I /Y Boilify.ofx.bundle "%PLUGIN_DIR%\Boilify.ofx.bundle"

echo Done. Restart DaVinci Resolve to use the plugin.
pause
