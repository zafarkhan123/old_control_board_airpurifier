#!/bin/bash

echo "formatting app folder..."
find ../main/app -name "*.h" -o -name "*.c" | xargs clang-format -i -style=file -fallback-style=none

echo "formatting middleware folder..."
find ../main/middleware -name "*.h" -o -name "*.c" | xargs clang-format -i -style=file -fallback-style=none

echo "formatting driver folder..."
find ../main/driver -name "*.h" -o -name "*.c" | xargs clang-format -i -style=file -fallback-style=none

CHANGES=`git diff`

if ! test -z "$CHANGES"; then
	echo ""
	echo "Changes proceed during format process: "
	echo ""
	echo "$CHANGES"
	exit 1
fi

echo ""
echo "No need to format anything"

exit 0