from PIL import Image

name = input("> ")
fn = "sprites/" + name + ".png"
im = Image.open(fn).convert("RGBA")

data = []
for y in range(im.size[1]):
    for x in range(im.size[0]):
        r, g, b, a = im.getpixel((x, y))
        print(r, g, b, a)
        if a < 255:
            data.append(0x34)
            data.append(0x12)
            continue
        rgb565 = (
            ((r & 0b11111000) << 8) | ((g & 0b11111100) << 3) | ((b & 0b11111000) >> 3)
        )
        data.append(rgb565 & 0b0000000011111111)
        data.append((rgb565 & 0b1111111100000000) >> 8)


with open("spritesbin/" + name + ".bin", "wb") as file:
    file.write(bytes(data))
