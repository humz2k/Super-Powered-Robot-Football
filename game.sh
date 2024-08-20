#!/usr/bin/env bash
cd build/package/release
leaks --atExit -- ./game
