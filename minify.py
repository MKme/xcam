#!/usr/bin/python3

import sys
import re

try:
    import minify_html
except ImportError:
    minify_html = None

if (len(sys.argv) <= 2):
    print('Usage: minify.py input.html output.html')
    sys.exit(1)

input_file = open(sys.argv[1], 'r')
output_file = open(sys.argv[2], 'w')

html = input_file.read()
input_file.close()

if minify_html:
    html_minified = minify_html.minify(html, minify_css=True)
else:
    html_minified = re.sub(r'>\s+<', '><', html)
    html_minified = re.sub(r'\s+', ' ', html_minified).strip()
output_file.write(html_minified)

output_file.close()
print('Done.')
