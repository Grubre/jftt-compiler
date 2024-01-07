#!/usr/bin/env python3

# the program should be run like ./run_compiler.py <input_file> [optional_test_file]

import sys
import os
import argparse
import subprocess
import tempfile

def parse_args():
    parser = argparse.ArgumentParser(description='Compiler for the language')
    parser.add_argument('input_file', type=str, help='input file to be compiled')
    parser.add_argument('test_file', nargs='?', type=str, help='optional test file to be used')
    parser.add_argument('--compiler', type=str, help='path to compiler executable')
    parser.add_argument('--vm', type=str, help='path to vm executable')

    return parser.parse_args()

def find_executable(path, executable_name): 
    for root, _, files in os.walk(path):
        for file in files:
            if file == executable_name:
                return os.path.join(root, file)
    return None

def main():
    args = parse_args()

    compiler = args.compiler if (args.compiler is not None) else find_executable('.', 'compiler')
    vm = args.vm if (args.vm is not None) else find_executable('.', 'maszyna-wirtualna')
    if compiler is None or not os.path.isfile(compiler):
        print('Could not find the compiler executable')
        sys.exit(1)
    if vm is None or not os.path.isfile(vm):
        print('Could not find the vm executable')
        sys.exit(1)

    if not os.path.isfile(args.input_file):
        print('Input file does not exist')
        sys.exit(1)
    input_file = args.input_file

    with tempfile.NamedTemporaryFile(delete=False) as tmp_file:
        compile_command = [compiler, input_file, tmp_file.name]
        subprocess.run(compile_command, check=True)

        vm_command = [vm, tmp_file.name]
        vm_output = subprocess.run(vm_command, check=True, capture_output=True).stdout

        if args.test_file:
            with open(args.test_file, 'r') as test_file:
                test_output = test_file.read()
                if vm_output == test_output:
                    print("Output matches the test file.")
                else:
                    print("Output does not match the test file.")

if __name__ == '__main__':
    main()
