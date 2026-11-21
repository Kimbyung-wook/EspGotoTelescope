import numpy as np
import matplotlib.pyplot as plt
from random import *

dt = 0.1
end_time = 50
a_mat = np.array([[1, dt, 0.5*dt*dt],
              [0, 1, dt],
              [0, 0, 1]])
b = np.array([0, 0, 1]).transpose()
f = np.eye(3)
h = np.array([[1,0,0],
              [0,0,1]])
q = np.diag([0.01, 0.01, 0.01])
r = np.diag([1, 0.1])

names = ['t','p','v','a']
filter = ['p_pre','v_pre','a_pre','p_post','v_post','a_post', 'q', 'r'] 
data = dict()
data['ekf'] = dict()
for name in names:
    data[name] = list()
for name in filter:
    data['ekf'][name] = list()

for idx in range(h.shape[1]):
    data['ekf']['q'] = list()
for idx in range(h.shape[0]):
    data['ekf']['r'] = list()
data['ekf']['q_p'] = list()
data['ekf']['q_v'] = list()
data['ekf']['q_a'] = list()
data['ekf']['r_p'] = list()
data['ekf']['r_v'] = list()
data['ekf']['r_a'] = list()
time = 0
p = 0
v = 0
a = 0

ekf = dict()
akf = dict()
ukf = dict()
for i in range(0,int(end_time/dt)+1):

    p_noise = uniform(-r[0][0], r[0][0])
    a_noise = uniform(-r[1][1], r[1][1])
    
    if(time < 10):
        a = 0
    elif(time < 20):
        a = 1
    elif(time < 30):
        a = -1
    elif(time < 40):
        a = 3
    else:
        a = 0

    time = time + dt
    p = p + v * dt
    v = v + a * dt
    data['t'].append(time)
    data['p'].append(p + p_noise)
    data['v'].append(v)
    data['a'].append(a + a_noise)
    y_m = np.array([p + p_noise * 0.5, a + a_noise * 0.5]).transpose()


    ############################################
    ## Linear Kalman Filtering
    ############################################
    if(i == 0):
        ekf['x_post'] = np.array([p, v, a]).transpose()
        ekf['p_post'] = np.eye(3)

    ### Time update
    u     = a
    # State vector
    ekf['x_pre'] = a_mat @ ekf['x_post'] #  + b * u
    # Error covariance matrix
    ekf['p_pre'] = a_mat @ (ekf['p_post'] @ a_mat.transpose()) + q

    ### Measurement update
    # Kalman Gain
    tmp = np.linalg.inv(np.matmul(np.matmul(h, ekf['p_pre']),h.transpose()) + r)
    ekf['k'] = np.matmul(np.matmul(ekf['p_pre'], h.transpose()), tmp)
    # state vector
    ekf['x_post'] = ekf['x_pre'] + np.matmul(ekf['k'], (y_m - np.matmul(h, ekf['x_pre'])))
    # Error covariance matrix
    ekf['p_post'] = np.matmul(np.eye(3) - np.matmul(ekf['k'], h), ekf['p_pre'])

    y_pre  = y_m - np.matmul(h, ekf['x_pre'])
    y_post = y_m - np.matmul(h, ekf['x_post'])

    ### Residual
    data['ekf']['q_p'].append(np.power(data['p'][-1] - ekf['x_pre'][0], 2))
    data['ekf']['q_v'].append(np.power(data['v'][-1] - ekf['x_pre'][1], 2))
    data['ekf']['q_a'].append(np.power(data['a'][-1] - ekf['x_pre'][2], 2))
    data['ekf']['r_p'].append(np.power(data['p'][-1] - ekf['x_post'][0], 2))
    data['ekf']['r_v'].append(np.power(data['v'][-1] - ekf['x_post'][1], 2))
    data['ekf']['r_a'].append(np.power(data['a'][-1] - ekf['x_post'][2], 2))

    ############################################
    ## Recording
    ############################################
    data['ekf']['p_pre'].append(ekf['x_pre'][0])
    data['ekf']['v_pre'].append(ekf['x_pre'][1])
    data['ekf']['a_pre'].append(ekf['x_pre'][2])
    data['ekf']['p_post'].append(ekf['x_post'][0])
    data['ekf']['v_post'].append(ekf['x_post'][1])
    data['ekf']['a_post'].append(ekf['x_post'][2])


plt.figure(1, figsize=[8, 8])
ax = plt.subplot(331)
ax.plot(data['t'], data['p'])
ax.plot(data['t'], data['ekf']['p_pre'], ":")
ax.plot(data['t'], data['ekf']['p_post'],'--')
ax.legend(['True', 'Pre', 'Post'])
ax.set_ylabel('Position [m]'); ax.grid()
ax = plt.subplot(332)
ax.plot(data['t'], data['ekf']['q_p'], ":")
ax.plot(data['t'], data['ekf']['r_p'],'--')
ax.set_ylabel('Delta Position [m]'); ax.grid()

ax = plt.subplot(334)
ax.plot(data['t'], data['v'])
ax.plot(data['t'], data['ekf']['v_pre'], ":")
ax.plot(data['t'], data['ekf']['v_post'],'--')
ax.legend(['True', 'Pre', 'Post'])
ax.set_ylabel('Velocity [m]'); ax.grid()
ax = plt.subplot(335)
ax.plot(data['t'], data['ekf']['q_v'], ":")
ax.plot(data['t'], data['ekf']['r_v'],'--')
ax.set_ylabel('Delta Velocity [m]'); ax.grid()

ax = plt.subplot(337)
ax.plot(data['t'], data['a'])
ax.plot(data['t'], data['ekf']['a_pre'], ":")
ax.plot(data['t'], data['ekf']['a_post'],'--')
ax.legend(['True', 'Pre', 'Post'])
ax.set_ylabel('Acceleration [m]'); ax.grid()
ax = plt.subplot(338)
ax.plot(data['t'], data['ekf']['q_a'], ":")
ax.plot(data['t'], data['ekf']['r_a'],'--')
ax.set_ylabel('Delta Acceleration [m]'); ax.grid()

plt.tight_layout()
plt.show()