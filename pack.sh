#!/bin/bash

SOURCE_DIR="template"
HEADER="payload.h"
OUTPUT_BIN="CFramework"

# Ensure the source directory exists
if [ ! -d "$SOURCE_DIR" ]; then
    echo "Error: Directory '$SOURCE_DIR' not found."
    exit 1
fi

echo "/* Automatically generated payload */" > $HEADER
echo "#include <stddef.h>" >> $HEADER

# 1. Find all files
FILES=$(find "$SOURCE_DIR" -type f -not -path '*/.*git*')

# 2. Generate Hex Arrays
for f in $FILES; do
    VAR_NAME="blob_$(echo "$f" | md5sum | cut -c1-16)"
    echo "unsigned char ${VAR_NAME}[] = {" >> $HEADER
    xxd -i < "$f" >> $HEADER
    echo "};" >> $HEADER
done

# 3. Generate Manifest
echo "typedef struct { const char* vpath; unsigned char* data; size_t size; const char* template_key; } FileEntry;" >> $HEADER
echo "FileEntry manifest[] = {" >> $HEADER

echo "--- Debugging Path Matching ---"
for f in $FILES; do
    VAR_NAME="blob_$(echo "$f" | md5sum | cut -c1-16)"
    SIZE=$(stat -c%s "$f")
    
    # Strip the SOURCE_DIR prefix precisely
    # If SOURCE_DIR is "template", this removes "template/"
    CLEAN_PATH=$(echo "$f" | sed "s|^$SOURCE_DIR/||")
    
    T_KEY="base" 

    # We use case matching here; it's often more reliable in Bash scripts
    case "$CLEAN_PATH" in
        *".engine/"*)
            T_KEY="core"
            ;;
        *"Dockerfile_sqlite"*|*"docker-compose.sqlite.yaml"*|*"Caddyfile"*)
            T_KEY="sqlite_docker"
            ;;
        *"Dockerfile_postgres"*|*"docker-compose.postgres.yaml"*|*"Caddyfile"*)
            T_KEY="postgres_docker"
            ;;
        *"Dockerfile_postgres_standalone"*|*"docker-compose.postgres-standalone.yaml"*|*"Caddyfile_standalone"*)
            T_KEY="postgres_standalone_docker"
            ;;
        *)
            T_KEY="base"
            ;;
    esac
    
    # This will print EVERY file found and the key assigned to it
    echo "File: $CLEAN_PATH  ->  Tag: $T_KEY"

    echo "    {\"$CLEAN_PATH\", ${VAR_NAME}, ${SIZE}, \"$T_KEY\"}," >> $HEADER
done

echo "    {NULL, NULL, 0, NULL}" >> $HEADER
echo "};" >> $HEADER

gcc generator.c -o "$OUTPUT_BIN"
echo "--------------------------------"
echo "Build complete."
