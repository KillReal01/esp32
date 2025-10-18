#!/bin/bash
set -e

source ~/esp-idf/export.sh

PROJECT_NAME=test
PROJECT_DIR=~/dev/esp32/${PROJECT_NAME}
PORT=/dev/ttyUSB0

cd "$PROJECT_DIR"

echo "=== Flash ==="
idf.py -p $PORT flash

echo "=== Finish ==="
