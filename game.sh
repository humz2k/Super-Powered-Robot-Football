#!/usr/bin/env bash
cd build/package
leaks --atExit -- ./game
