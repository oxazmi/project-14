import imclab, time

a = imclab.iMCLab()
print(a.version())

for i in range(0, 101, 10):
    print("LED", i)
    a.LED(i)
    time.sleep(0.3)

for i in range(100, -1, -10):
    print("LED", i)
    a.LED(i)
    time.sleep(0.3)

a.LED(0)
a.close()
