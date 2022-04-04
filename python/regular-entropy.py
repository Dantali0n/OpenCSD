# Shannon Entropy of a file
# FB - 201012153
import sys
import math
if len(sys.argv) != 2:
    print('Usage: file_entropy.py [path]filename')
    sys.exit()

f = open(sys.argv[1], 'rb')
byteArr = bytearray(f.read())
f.close()
fileSize = len(byteArr)

print('File size in bytes:', fileSize)

freqList = [0] * 256
for b in byteArr:
    freqList[b] += 1

ent = 0.0
for f in freqList:
    if f > 0:
        freq = float(f) / fileSize
        ent = ent + freq * math.log(freq, 2)
ent = -ent
print('Shannon entropy (min bits per byte-character):', ent)
print('Min possible file size assuming max theoretical compression efficiency:')
print((ent * fileSize) / 8, 'bytes')