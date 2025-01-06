# poi_image_converter.py
# This script converts fan shaped image to poi shaped image.

# Usage
# $ python ./poi_image_converter.py image1_circle.png image1.png

import sys
import numpy
import math
from PIL import Image

src_filename = 'src.png'
dst_filename = 'dst.png'

args = sys.argv
if 2 <= len(args):
  src_filename = args[1]
if 3 <= len(args):
  dst_filename = args[2]

src_img = Image.open(src_filename)

src_width, src_height = src_img.size

src_x0 = src_width / 2
src_y0 = src_height / 2

dst_width = int(src_width / 2)
if src_width > src_height:
  dst_width = int(src_height / 2)

dst_height = int(dst_width * 2 * numpy.pi)
dst_img = Image.new("RGB", (dst_width, dst_height))


for y in range(dst_height):
  for r in range(dst_width):
    theta = y / dst_width
    src_x = src_x0 + r * math.cos(theta)
    src_y = src_y0 + r * math.sin(theta)

    dst_img.putpixel((r,y), src_img.getpixel((src_x, src_y)))
  
dst_img.save(dst_filename)