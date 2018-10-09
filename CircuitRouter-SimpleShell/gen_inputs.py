#!/usr/bin/python3

input_list = [
    'random-x128-y128-z3-n128.txt',
    'random-x128-y128-z3-n64.txt',
    'random-x128-y128-z5-n128.txt',
    'random-x256-y256-z3-n256.txt',
    'random-x256-y256-z5-n256.txt',
    'random-x32-y32-z3-n64.txt',
    'random-x32-y32-z3-n96.txt',
    'random-x48-y48-z3-n48.txt',
    'random-x48-y48-z3-n64.txt',
    'random-x512-y512-z7-n512.txt',
    'random-x64-y64-z3-n48.txt',
    'random-x64-y64-z3-n64.txt'
]

for _ in input_list:
    print("run ../CircuitRouter-SeqSolver/inputs/"+_);
