import os
import sys
import subprocess

RED = "\033[31m"
GREEN = "\033[32m"
RESET = "\033[0m"

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("usage: <program path>")
        sys.exit(1)
    try:
        tinycoro_path = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        process = subprocess.run(
            [f"{tinycoro_path}/scripts/perf_svg.sh", sys.argv[1], tinycoro_path],
            capture_output=True,
            text=True,
        )
        if process.returncode != 0:
            print(f"{RED}{tinycoro_path}/scripts/perf_svg.sh run error!{RESET}")
            print(f"STDOUT:", process.stdout)
            print(f"STDERR:", process.stderr)
            sys.exit(0)
        print(f"perf run success! output svg path: {tinycoro_path}/temp/perf.svg")
    except Exception as e:
        print(f"{RED} exception occur! detail: {e}{RESET}")
        sys.exit(1)
