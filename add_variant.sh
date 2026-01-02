#!/bin/bash

NEW_TEMPLATE_DIR=$1
VARIANT_NAME=$2
HEADER="payload.h"
C_FILE="generator.c"
OUTPUT="CFramework"

if [ -z "$1" ] || [ -z "$2" ]; then
    echo "Usage: ./add_variant.sh <directory> <variant_name>"
    exit 1
fi

# 1. Create a temporary manifest file to hold the NEW entries
TEMP_ENTRIES="entries.tmp"
echo "" > $TEMP_ENTRIES

# 2. Generate Hex Blobs and append them to the TOP of the file (before the manifest)
# We use a marker /* BLOBS_END */ to know where to insert
FILES=$(find "$NEW_TEMPLATE_DIR" -type f -not -path '*/.*git*')

for f in $FILES; do
    VAR_NAME="blob_$(echo "$f" | md5sum | cut -c1-16)"
    SIZE=$(stat -c%s "$f")
    CLEAN_PATH=${f#$NEW_TEMPLATE_DIR/} 

    # Insert the blob at the very top of the header
    {
        echo "unsigned char ${VAR_NAME}[] = {"
        xxd -i < "$f"
        echo "};"
    } | cat - "$HEADER" > temp_header.h && mv temp_header.h "$HEADER"

    # Save the manifest entry to our temp file
    echo "    {\"$CLEAN_PATH\", ${VAR_NAME}, ${SIZE}, \"$VARIANT_NAME\"}," >> $TEMP_ENTRIES
done

# 3. Insert the new entries into the manifest array
# This looks for the sentinel line and inserts BEFORE it
sed -i "/{NULL, NULL, 0, NULL}/e cat $TEMP_ENTRIES" "$HEADER"

# 4. Clean up
rm $TEMP_ENTRIES

# 5. Recompile
gcc "$C_FILE" -o "$OUTPUT"
echo "------------------------------------------------"
echo "Success! Variant '$VARIANT_NAME' embedded and recompiled."
