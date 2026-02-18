#!/bin/bash
set -e

case "$(uname -s)" in
    Darwin)
        PLUGIN_DIR="/Library/OFX/Plugins"
        ;;
    Linux)
        PLUGIN_DIR="/usr/OFX/Plugins"
        ;;
    *)
        echo "Run install.bat on Windows"
        exit 1
        ;;
esac

echo "Installing Boilify to $PLUGIN_DIR"

sudo mkdir -p "$PLUGIN_DIR"
sudo cp -R Boilify.ofx.bundle "$PLUGIN_DIR/"

echo "Done. Restart DaVinci Resolve to use the plugin."
