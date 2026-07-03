#!/usr/bin/env bash
# Install the PulseLinux git hooks into .git/hooks.
# Run once after cloning: bash scripts/install-hooks.sh

set -euo pipefail
REPO_ROOT="$(git rev-parse --show-toplevel)"
HOOKS_SRC="$REPO_ROOT/scripts/hooks"
HOOKS_DST="$REPO_ROOT/.git/hooks"

for hook in "$HOOKS_SRC"/*; do
  name="$(basename "$hook")"
  dst="$HOOKS_DST/$name"
  ln -sf "$hook" "$dst"
  echo "Installed: .git/hooks/$name -> scripts/hooks/$name"
done
echo "All hooks installed."
