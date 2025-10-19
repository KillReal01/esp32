#!/bin/bash
set -e

if [ -z "$1" ]; then
    echo "Usage: $0 <project_name>"
    exit 1
fi

PROJECT_NAME="$1"
PROJECT_DIR=~/dev/esp32/${PROJECT_NAME}
PORT=/dev/ttyUSB0
TARGET=esp32

if [ ! -d "$PROJECT_DIR" ]; then
    echo "Error: Project directory '$PROJECT_DIR' does not exist."
    exit 1
fi

source ~/esp-idf/export.sh

cd "$PROJECT_DIR"

echo "=== Build project ==="
idf.py set-target $TARGET

idf.py fullclean build

echo "=== Finish ==="
