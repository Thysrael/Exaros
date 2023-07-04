import os
import re
import difflib
import subprocess
import sys


user_output_path = "../serial.log"
with open(user_output_path, "r") as f:
    user_output = f.read().replace("\n\n", "\n")

test_path = "./testcases"


def judge(test_name):
    begin = "### " + test_name + " BEGIN ###"
    end = "### " + test_name + " END ###"
    re_str = re.compile(begin + "(.*)" + end, re.S)
    reg = re_str.search(user_output)
    output = reg.group()
    # 有 .py 就使用 .py 来判断
    # 没有 .py 就直接使用 diff
    if os.path.exists(os.path.join(test_path, test_name + ".py")):
        proc = subprocess.Popen(
            args=["python", os.path.join(test_path, test_name + ".py")],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        stdout, stderr = proc.communicate(input=output.encode())
        if stderr.decode():
            print("[TEST] " + test_name + " ERROR! [STDERR]:")
            print(stderr.decode())
            exit()
        if stdout.decode():
            print("[TEST] " + test_name + " ERROR! [STDOUT]:")
            print(output)
            exit()
    else:
        with open(os.path.join(test_path, test_name + ".out"), "r") as f:
            std_output = f.read()
        diff = difflib.unified_diff(
            std_output.splitlines(),
            output.splitlines(),
            lineterm="",
            fromfile=test_name + "_std",
            tofile=test_name + "_user",
        )
        if len(diff):
            print("[TEST] " + test_name + " ERROR! [DIFFOUT]:")
            print(diff)
            exit()
    print("[TEST] " + test_name + " PASS!")


if len(sys.argv) > 1:
    for file in sys.argv[1:]:
        judge(file)
else:
    files = os.listdir(test_path)
    for file in files:
        reg = re.match(r"(.*).out", file)
        if reg:
            judge(reg.group(1))
