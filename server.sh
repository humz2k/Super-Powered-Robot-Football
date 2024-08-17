#!/usr/bin/env bash
cd build
leaks --atExit -- ./server server_config.ini