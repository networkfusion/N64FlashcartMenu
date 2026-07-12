#!/bin/bash
# Helper script to build and run host-side unit tests within the devcontainer
# Usage: ./run_tests.sh

set -e

IMAGE_NAME="n64flashcartmenu-sc64deployer"
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

echo "Building devcontainer image..."
docker build --progress=plain -t "$IMAGE_NAME" -f .devcontainer/flashcart/Dockerfile.sc64deployer . > /dev/null 2>&1 || true

echo "Running unit tests..."
docker run --rm -v "${PROJECT_DIR}:/workspaces/N64FlashcartMenu" \
    -w /workspaces/N64FlashcartMenu/tests \
    "$IMAGE_NAME" \
    bash -lc "make -B test"

echo ""
echo "✓ All tests passed!"
