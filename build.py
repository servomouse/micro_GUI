import os
import sys
import subprocess
import time
import glob
from termcolor import colored
from pathlib import Path    # to check if a directory exists

compiler_path = "gcc"


compiler_flags = ["-Iinclude -lm", "-Wl,-lm"]


def compile(input_file:str) -> int:
    ''' returns 1 if error, 0 if ok '''
    print(f"file: {input_file}", flush=True)
    time.sleep(0.1)
    output_file = f"temp_files/{input_file.split('/')[-1].split('.')[0]}.o"
    c_string = f'{compiler_path} {input_file} {" ".join(compiler_flags)} -c -o {output_file}'
    return subprocess.call(c_string,shell=True)


def compile_all(files:list):
    # print(f"files to compile: {' '.join(files)}")
    for item in files:
        if compile(item) == 1:
            print(colored(f"{item} compilation error", 'red'))
            sys.exit(0)
    print(colored("compilation completed successfully", 'green'))
    return 0


def link_all(files:list):
    l_string = f'{compiler_path} {" ".join(files)} {" ".join(compiler_flags)} -o main'
    retval = subprocess.call(l_string, shell=True)
    if retval == 1:
        print(colored("linking error", 'red'))
        sys.exit(0)
    return retval


def get_files(path:str, filetype:str)->list:
    result = []

    for x in os.walk(path):
        for y in glob.glob(os.path.join(x[0], f'*{filetype}')):
            temp = y[len(path)+1:].replace("\\", "/")
            result.append(temp)
    return result


def build():
    Path("temp_files").mkdir(parents=True, exist_ok=True) # create temp directory if there is no one
    workdir = os.path.abspath(os.getcwd())
    if 0 == compile_all(get_files(workdir, ".c")) and 0 == link_all(get_files(workdir, ".o")):
        subprocess.call('rm -r temp_files/*', shell=True)
        return 1
    return 0

if __name__ == '__main__':
    build()

