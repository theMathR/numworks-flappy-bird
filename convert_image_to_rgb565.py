import os

from PIL import Image

if not os.path.isdir("spritesbin"):
    os.makedirs("spritesbin")

for name in os.listdir("sprites"):
    fn = "sprites/" + name
    im = Image.open(fn).convert("RGBA")

    data = []
    for y in range(im.size[1]):
        for x in range(im.size[0]):
            r, g, b, a = im.getpixel((x, y))
            if a < 255:
                data.append(0x34)
                data.append(0x12)
                continue
            rgb565 = (
                ((r & 0b11111000) << 8)
                | ((g & 0b11111100) << 3)
                | ((b & 0b11111000) >> 3)
            )
            data.append(rgb565 & 0b0000000011111111)
            data.append((rgb565 & 0b1111111100000000) >> 8)

    with open("spritesbin/" + name[:-4] + ".bin", "wb") as file:
        file.write(bytes(data))
