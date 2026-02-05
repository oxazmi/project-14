import imclab, time

a = imclab.iMCLab()
print(a.version())

p = 80
print("Set OP", p, "reply:", a.op(p))
time.sleep(8)        

print(a.stop())
a.close()
