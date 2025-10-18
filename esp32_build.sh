#!/bin/bash
set -e

source ~/esp-idf/export.sh

PROJECT_NAME=test
PROJECT_DIR=~/dev/esp32/${PROJECT_NAME}
PORT=/dev/ttyUSB0
TARGET=esp32

cd "$PROJECT_DIR"

echo "=== Build project ==="
idf.py set-target $TARGET
idf.py build

echo "=== Finish ==="
