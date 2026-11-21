import numpy as np
import matplotlib.pyplot as plt

from random import *

uniform(-1.0, 1.0)

names = ['t', 'ax', 'ay', 'az', 'wx', 'wy', 'wz', 'dphi', 'dtheta', 'dpsi', 'phi', 'theta', 'psi']
data = dict()
for name in data.keys:
    data[name] = list()

noise_acc = 0.1
noise_gyro = np.deg2rad(0.5)

ax = 0
ay = 0
az = 0
wx = 0
wy = 0
wz = 0
phi = 0
theta = 0
psi = 0
dphi_in = 0
dtheta_in = 0
dpsi_in = 0
dt = 0.1

class att:
    def __init__(self):
        self.phi   = 0
        self.theta = 0
        self.psi   = 0

    def update(self, dt, dphi_in, dtheta_in, dpsi_in):
        wx = dphi_in                             - np.sin(self.psi) * dpsi_in
        wy =        np.cos(self.phi) * dtheta_in + np.cos(self.theta) * np.sin(self.phi) * dpsi_in
        wz =       -np.sin(self.phi) * dtheta_in + np.cos(self.theta) * np.cos(self.phi) * dpsi_in

        self.phi    += dt * dphi_in
        self.theta  += dt * dtheta_in
        self.psi    += dt * dpsi_in

        ax = (9.80665) * (-np.sin(self.theta))
        ay = (9.80665) * ( np.sin(self.phi) * np.cos(self.theta))
        az = (9.80665) * ( np.cos(self.phi) * np.cos(self.theta))

        return [ax, ay, az, wx, wy, wz, self.phi, self.theta, self.psi]

class ukf:
    def __init__(self):
        pass

    def update(self):
        pass

model = att()

for idx in np.arange(1, 50/dt, 1):
    data['t'].append(idx * dt)

    ###############################################################
    ## Changing
    ###############################################################
    if(idx < 10 / dt):
        dphi_in   = 0
        dtheta_in = 0
        dpsi_in   = 0
    elif(idx < 20 / dt):
        dphi_in   = np.deg2rad(3)
        dtheta_in = 0
        dpsi_in   = 0
    elif(idx < 30 / dt):
        dphi_in   = 0
        dtheta_in = np.deg2rad(3)
        dpsi_in   = 0
    elif(idx < 40 / dt):
        dphi_in   = 0
        dtheta_in = 0
        dpsi_in   = 0
    else:
        dphi_in   = 0
        dtheta_in = 0
        dpsi_in   = 0

        
    ###############################################################
    ## Simulation
    ###############################################################
    [ax, ay, az, wx, wy, wz, phi, theta, psi] = model.update(dt, dphi_in, dtheta_in, dpsi_in)
    data['ax'].append(ax + uniform(-noise_acc, noise_acc))
    data['ay'].append(ay + uniform(-noise_acc, noise_acc))
    data['az'].append(az + uniform(-noise_acc, noise_acc))
    data['wx'].append(wx + uniform(-noise_gyro, noise_gyro))
    data['wy'].append(wy + uniform(-noise_gyro, noise_gyro))
    data['wz'].append(wz + uniform(-noise_gyro, noise_gyro))
    data['dphi'].append(dphi_in)
    data['dtheta'].append(dtheta_in)
    data['dpsi'].append(dpsi_in)
    data['phi'].append(phi)
    data['theta'].append(theta)
    data['psi'].append(psi)

    ###############################################################
    ## Kalman Filtering
    ###############################################################
    L_dim = 2
    alpha = 0.001 # spread of sigma point, usually set to a small positive value
    kappa = 0 # alpha scaling param
    beta = 2.0 # distribution of x, 2.0 is optimal as x is gaussian distributions
    lamda = alpha * alpha * (L_dim + kappa) - L_dim

    def f_function(p, q, r, phi, theta, psi):
        mat = np.array[[1, np.tan(theta)*np.sin(phi), np.tan(theta)*np.cos(phi)],
                       [0, np.cos(phi), -np.sin(phi)],
                       [0, np.sin(phi)/np.cos(theta), np.cos(phi)/np.cos(theta)]]
        # f = mat * np.array[[p], [q], [r]]
        return mat
    f_k = f_function(wx, wy, wz, phi, theta, psi)

    if(idx == 1):
        nu_m = lamda / (L_dim + lamda)
        nu_c = lamda / (L_dim + lamda) + (1 - alpha * alpha + beta)
    else:
        nu_m = 1 / (2 * (L_dim + lamda))
        nu_c = 1 / (2 * (L_dim + lamda))



###############################################################
## Figure
###############################################################
plt.figure(1,figsize=[6,8])
ax = plt.subplot(411)
ax.plot(data['t'], data['ax'])
ax.plot(data['t'], data['ay'])
ax.plot(data['t'], data['az'])
ax.set_xlabel('Time [sec]')
ax.set_ylabel('Acc [m/s2]')
ax.grid()
ax = plt.subplot(412)
ax.plot(data['t'], np.rad2deg(data['wx']))
ax.plot(data['t'], np.rad2deg(data['wy']))
ax.plot(data['t'], np.rad2deg(data['wz']))
ax.set_xlabel('Time [sec]')
ax.set_ylabel('Angular Rate [Deg/s]')
ax.grid()
ax = plt.subplot(413)
ax.plot(data['t'], np.rad2deg(data['dphi']))
ax.plot(data['t'], np.rad2deg(data['dtheta']))
ax.plot(data['t'], np.rad2deg(data['dpsi']))
ax.set_xlabel('Time [sec]')
ax.set_ylabel('Eulerrate [Deg/s]')
ax.grid()
ax = plt.subplot(414)
ax.plot(data['t'], np.rad2deg(data['phi']))
ax.plot(data['t'], np.rad2deg(data['theta']))
ax.plot(data['t'], np.rad2deg(data['psi']))
ax.legend(['phi', 'theta', 'psi'])
ax.set_xlabel('Time [sec]')
ax.set_ylabel('Euler Angle [Deg]')
ax.grid()
plt.tight_layout()
plt.show()



