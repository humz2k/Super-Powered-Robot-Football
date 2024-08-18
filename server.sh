#!/usr/bin/env bash
cd build/package
leaks --atExit -- ./server server_cfg.ini