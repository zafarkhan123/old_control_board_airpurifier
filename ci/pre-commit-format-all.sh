#!/bin/bash

echo "formatting app folder..."
find main/app -name "*.h" -o -name "*.c" | xargs clang-format -i -style=file -fallback-style=none

echo "formatting middleware folder..."
find main/middleware -name "*.h" -o -name "*.c" | xargs clang-format -i -style=file -fallback-style=none

echo "formatting driver folder..."
find main/driver -name "*.h" -o -name "*.c" | xargs clang-format -i -style=file -fallback-style=none

CHANGES=`git diff`

if ! test -z "$CHANGES"; then
    echo "    Pipeline will fail if you push unformatted code."
    echo "    Knowing that, you can bypass this check locally with:"
    echo ""
    echo "    git commit --no-verify"
    echo ""
	exit 1
else
    echo "pre-commit-format-all:"
    echo "    Code formatted correctly."
    echo ""
fi

exit 0