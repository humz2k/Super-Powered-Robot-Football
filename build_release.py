import os
import shutil

os.chdir(os.path.dirname(__file__))

print("LOG: working in " + os.curdir)

BUILD_DIR = "build"
PACKAGE_DIR = "package"
RELEASE_DIR = "release"
RELEASE_ZIP = "release"

print("LOG: building game")

os.system(f"rm -rf {BUILD_DIR}")

os.system(f"cmake -B {BUILD_DIR}/")
os.system(f"cmake --build {BUILD_DIR} --target game --config Release -j")
os.system(f"cmake --build {BUILD_DIR} --target server --config Release -j")
os.system(f"cmake --build {BUILD_DIR} --target editor --config Release -j")

print(f"LOG: copying to {RELEASE_DIR}")

root_src_dir = os.path.join(BUILD_DIR,PACKAGE_DIR)
root_dst_dir = RELEASE_DIR  #Path to the destination folder
os.system(f"rm -rf {RELEASE_DIR}")
try:
    os.remove(RELEASE_ZIP + ".zip")
except:
    pass

for src_dir, dirs, files in os.walk(root_src_dir):
    dst_dir = src_dir.replace(root_src_dir, root_dst_dir, 1)
    if not os.path.exists(dst_dir):
        os.makedirs(dst_dir)
    for file_ in files:
        src_file = os.path.join(src_dir, file_)
        dst_file = os.path.join(dst_dir, file_)
        if os.path.exists(dst_file):
            os.remove(dst_file)
        shutil.copy(src_file, dst_dir)

print(f"LOG: zipping to {RELEASE_ZIP}.zip")

shutil.make_archive(RELEASE_ZIP, 'zip', root_dst_dir)