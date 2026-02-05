import imclab
import time

a = imclab.iMCLab()    # harusnya auto detect COM5
print(a.version())     # pakai kurung ya

print("LED ON")
a.LED(100)
time.sleep(1)

print("LED OFF")
a.LED(0)

a.close()
