#!/usr/bin/env bash
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
EXECUTABLE="$PROJECT_ROOT/build/bin/debug/typespeed"
if [[ -x "$EXECUTABLE" ]]; then
  exec "$EXECUTABLE"
else
  echo "No executable found. Run create.sh first."
  exit 1
fi
