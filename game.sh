#!/usr/bin/env bash
cd build
leaks --atExit -- ./game
