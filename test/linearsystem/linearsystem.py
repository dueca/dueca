from control import matlab as ml
import numpy as np

# verification
dt = 0.02
wn = 0.066666667
zn = 0.01

s = ml.tf([1,0],[1])
h = (1 + 2*zn/wn*s + s**2/wn**2) / (1 + 2/wn*s + s**2/wn**2)
print 'num', h.num, '\nden', h.den

sys = ml.ss(h)
print 'A', sys.A, '\nB', sys.B, '\nC:', sys.C, '\nD', sys.D

print sys.pole()
print sys.zero()

sysd = ml.c2d(sys, dt)
print sysd.pole()
print sysd.zero()

print 'Psi', sysd.A, '\nPhi', sysd.B,

# implementation
A = np.matrix([[ 0, -0.004444444444444444], [1, -0.133333333333333333]])
B = np.matrix([[0],[-0.132]])
C = np.matrix([[0, 1]])
D = np.matrix([[1]])
sysi = ml.ss(A, B, C, D)
print sysi.pole()
print sysi.zero()
print h.pole()
print h.zero()

a = np.array(h.den[0][0])
b = np.array(h.num[0][0])
a = a/a[0]
b = b/b[0]
#print np.linalg.eig(A)
