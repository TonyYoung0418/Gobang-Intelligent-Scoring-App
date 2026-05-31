import re
def replace(matched):
    return '{ ' + matched.group(1)
filename = "tools.h"
try:
    data = open(filename, "r", encoding="utf-8").read()
    result = re.sub(r"(//.+)\s*\{", replace, data)
    result = re.sub(r"\s+\{", " {", result)
    open(filename, "w", encoding="utf-8").write(result)
except UnicodeDecodeError:
    data = open(filename, "r").read()
    result = re.sub(r"(//.+)\s*\{", replace, data)
    result = re.sub(r"\s+\{", " {", result)
    open(filename, "w").write(result)
