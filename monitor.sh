#!/bin/bash
set -e

source ~/esp-idf/export.sh

if [ -z "$1" ]; then
    echo "Usage: $0 <project_name>"
    exit 1
fi

PROJECT_NAME="$1"
PROJECT_DIR=~/dev/esp32/${PROJECT_NAME}
PORT=/dev/ttyUSB0

if [ ! -d "$PROJECT_DIR" ]; then
    echo "Error: Project directory '$PROJECT_DIR' does not exist."
    exit 1
fi

cd "$PROJECT_DIR"

echo "=== Monitor ==="

idf.py -p $PORT monitor

echo "=== Finish ==="
