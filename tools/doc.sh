#!/usr/bin/env bash

set -e

# Get the project root directory relative to this script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR=$(git -C "$SCRIPT_DIR" rev-parse --show-toplevel)
DOC_DIR="$PROJECT_DIR/doc"
TMP_DIR="$PROJECT_DIR/.tmp"
OUT_DIR="$TMP_DIR/doc"
RES_DIR="$PROJECT_DIR/res"

# Default stylesheet
STYLESHEET="$RES_DIR/dark-doc.css"

mkdir -p "$OUT_DIR"

compile_file() {
    local file="$1"
    if [ ! -f "$file" ]; then
        echo "Error: File not found: $file"
        return 1
    fi
    local filename=$(basename "$file" .adoc)
    echo "  $filename.adoc -> $filename.html"

    local asciidoc_args=("-b" "html5")
    if [ -f "$STYLESHEET" ]; then
        asciidoc_args+=("-a" "stylesheet=$STYLESHEET")
    fi

    asciidoc "${asciidoc_args[@]}" -o "$OUT_DIR/$filename.html" "$file"
}

show_help() {
    echo "Usage: $0 [OPTIONS] [FILE]"
    echo ""
    echo "Compiles AsciiDoc (.adoc) files into HTML."
    echo ""
    echo "Arguments:"
    echo "  FILE    Path to the AsciiDoc file to compile. If omitted, all .adoc"
    echo "          files in the doc/ directory will be compiled."
    echo ""
    echo "Options:"
    echo "  -s, --stylesheet PATH    Path to a custom CSS stylesheet."
    echo "  -h, --help               Show this help message and exit."
}

# Parse options
FILE=""
while [[ $# -gt 0 ]]; do
    case "$1" in
        -s|--stylesheet)
            STYLESHEET="$2"
            if [ ! -f "$STYLESHEET" ]; then
                echo "Error: Stylesheet not found: $STYLESHEET"
                exit 1
            fi
            shift 2
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        -*)
            echo "Unknown option: $1"
            show_help
            exit 1
            ;;
        *)
            FILE="$1"
            shift
            ;;
    esac
done

if [ -n "$FILE" ]; then
    echo "Compiling documentation for $FILE..."
    echo "Using stylesheet: $STYLESHEET"
    compile_file "$FILE"
else
    echo "Compiling all documentation in $DOC_DIR..."
    echo "Using stylesheet: $STYLESHEET"
    find "$DOC_DIR" -maxdepth 1 -name "*.adoc" | while read -r file; do
        if [ -f "$file" ]; then
            compile_file "$file" || echo "Warning: Failed to compile $file"
        fi
    done
fi

echo "Documentation compiled to $OUT_DIR"
