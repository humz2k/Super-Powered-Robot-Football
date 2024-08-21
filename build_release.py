import os

os.system("rm -rf build")
#os.system("mkdir build")
os.system("cmake -B build/")
os.system("cmake --build build --target game --config Release")
os.system("cmake --build build --target server --config Release")
os.system("cmake --build build --target editor --config Release")
