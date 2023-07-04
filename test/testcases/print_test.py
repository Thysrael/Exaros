import sys

input = sys.stdin.readlines()[1:-1]
if len(input) != 1 or input[0].strip() != "hello world!":
    print("error")
