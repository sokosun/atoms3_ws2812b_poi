# rawdata_converter.py
# This script prints RGB value for each pixel.

# Usage
# $ python ./rawdata_converter.py image1.png > image1.h

import sys
from PIL import Image

filename = 'src.png'
name = 'src'
resolution = 72;
args = sys.argv
if 2 <= len(args):
  filename = args[1]
  tmp = args[1].split('.')
  name = tmp[0]

img = Image.open(filename)
w, h = img.size
resized_img = img.resize((resolution, h * resolution // w))
width, height = resized_img.size

print("#include <stdint.h>")
print("constexpr uint32_t ", end="")
print(name, end="")
print("[][", end="")
print(width, end="")
print("] = {")
for y in range(height):
  print("  {")
  for x in range(width):
    r, g, b = resized_img.getpixel((x, y))
    rgb = (r << 16) | (g << 8) | b
  
    if x == width - 1:
      print("    0x%x" % rgb)
    else:
      print("    0x%x," % rgb)

  if y == height - 1:
    print("  }")
  else:
    print("  },")

print("};")
