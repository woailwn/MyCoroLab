import sys
import xml.etree.ElementTree as ET

RED = "\033[31m"
GREEN = "\033[32m"
RESET = "\033[0m"

if __name__ == "__main__":
    try:
        tree = ET.parse(sys.argv[1])
        root = tree.getroot()
        for element in root:
            if element.tag == "error":
                print(
                    f"{RED}[ERROR] exist memleak, you can see the detail report path in {sys.argv[1]}{RESET}"
                )
                sys.exit(1)
        print(f"{GREEN}[PASS] pass memcheck test{RESET}")
    except Exception as e:
        print(
            f"{RED}[EXCEPTION] exception occur when parse valgrind output, detail: {e}{RESET}"
        )
        sys.exit(1)
