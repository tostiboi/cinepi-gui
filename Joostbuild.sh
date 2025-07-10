#!/bin/bash

rm -r build
meson setup build && ninja -C build
