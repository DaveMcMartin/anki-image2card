#!/bin/bash

# Bundle Tesseract and Leptonica libraries into macOS .app bundle
# Usage: bundle_ocr_libs.sh <executable_path>

set -e

EXECUTABLE="$1"
FRAMEWORKS_DIR="$(dirname "$EXECUTABLE")/../Frameworks"

echo "Bundling OCR libraries for: $EXECUTABLE"
echo "Frameworks directory: $FRAMEWORKS_DIR"

# Create Frameworks directory if it doesn't exist
mkdir -p "$FRAMEWORKS_DIR"

# Function to copy and fix a library
bundle_library() {
    local LIB_PATH="$1"
    local LIB_NAME=$(basename "$LIB_PATH")
    local DEST="$FRAMEWORKS_DIR/$LIB_NAME"

    if [ ! -f "$LIB_PATH" ]; then
        echo "Warning: Library not found: $LIB_PATH"
        return
    fi

    echo "Copying $LIB_NAME..."
    cp "$LIB_PATH" "$DEST"
    chmod u+w "$DEST"

    # Update the library's install name
    install_name_tool -id "@rpath/$LIB_NAME" "$DEST"

    # Fix dependencies within the library
    otool -L "$DEST" | grep -E "tesseract|leptonica" | awk '{print $1}' | while read dep; do
        if [ "$dep" != "$LIB_NAME" ]; then
            DEP_NAME=$(basename "$dep")
            install_name_tool -change "$dep" "@rpath/$DEP_NAME" "$DEST" 2>/dev/null || true
        fi
    done
}

# Find and bundle Tesseract library
TESS_LIB=$(otool -L "$EXECUTABLE" | grep tesseract | awk '{print $1}' | head -1)
if [ -n "$TESS_LIB" ]; then
    bundle_library "$TESS_LIB"
    TESS_NAME=$(basename "$TESS_LIB")
    install_name_tool -change "$TESS_LIB" "@rpath/$TESS_NAME" "$EXECUTABLE"
fi

# Find and bundle Leptonica library
LEPT_LIB=$(otool -L "$EXECUTABLE" | grep leptonica | awk '{print $1}' | head -1)
if [ -n "$LEPT_LIB" ]; then
    bundle_library "$LEPT_LIB"
    LEPT_NAME=$(basename "$LEPT_LIB")
    install_name_tool -change "$LEPT_LIB" "@rpath/$LEPT_NAME" "$EXECUTABLE"
fi

echo "OCR libraries bundled successfully!"
