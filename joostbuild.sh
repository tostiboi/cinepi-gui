#!/bin/bash
cd "$(dirname "$0")"
rm -r build
meson setup build && ninja -C build

