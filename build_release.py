import os
import shutil

os.system("rm -rf build")
os.system("cmake -B build/")
os.system("cmake --build build --target game --config Release")
os.system("cmake --build build --target server --config Release")
os.system("cmake --build build --target editor --config Release")

root_src_dir = 'build/package'    #Path/Location of the source directory
root_dst_dir = 'release'  #Path to the destination folder
os.system('rm -rf release')
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

shutil.make_archive('release', 'zip', root_dst_dir)