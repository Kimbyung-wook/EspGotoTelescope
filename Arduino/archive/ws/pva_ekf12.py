import numpy as np
import matplotlib.pyplot as plt
from random import *
import copy

dt = 0.1
end_time = 50
a_mat = np.array([[1, dt, 0.5*dt*dt],
                  [0,  1, dt],
                  [0,  0, 1]])
b = np.array([0.5*dt*dt, dt, 1]).transpose()
f = np.eye(3)
h = np.array([[1,0,0],
              [0,0,1]])
q_init = np.array([1, 0.05, 0.05])
r_init = np.array([1, 0.1])

filter = dict()
filter['t'] = list()
state_list = ['s','v','a']
input_list = ['a']
output_list = state_list
for name in state_list:
    filter[name] = list()
for name in state_list:
    filter[name + "_pre"] = list()
for name in state_list:
    filter[name + "_post"] = list()
for name in state_list:
    filter["q_" + name] = list()
for name in state_list:
    filter["r_" + name] = list()
for name in state_list:
    filter["d1_" + name] = list()
for name in state_list:
    filter["d2_" + name] = list()
for name in input_list:
    filter[name + "_in"] = list()

ekf = copy.deepcopy(filter)

# print(ekf)

model = dict()
model['t'] = list()
for name in state_list:
    model[name] = list()
model['s_true'] = list()
model['v_true'] = list()
model['a_true'] = list()

time = 0
p = 0
v = 0
a = 0
tau = 1.0
alpha = dt/tau
a_prev = 0
a_cmd = 1.0

for i in range(0,int(end_time/dt)+1):

    p_noise = uniform(-r_init[0], r_init[0])
    a_noise = uniform(-r_init[1], r_init[1])
    
    if(time < 10):
        a = 0
    elif(time < 20):
        a = a_cmd
    elif(time < 30):
        a = -a_cmd
    elif(time < 40):
        a = a_cmd
    else:
        a = 0

    time = time + dt
    p = p + v * dt
    v = v + a * dt
    model['t'].append(time)
    model['s'].append(p)
    model['v'].append(v)
    model['a'].append(a)
    y_m = np.array([p + p_noise * 0.5, a + a_noise * 0.5]).transpose()
    model['s_true'].append(y_m[0])
    model['a_true'].append(y_m[1])


    ############################################
    ## Linear Kalman Filtering
    ############################################
    ekf['t'].append(time)
    if(i == 0):
        ekf['x_post'] = np.array([p, v, a]).transpose()
        ekf['p_post'] = np.eye(3)
        ekf['x_pre'] = np.array([p, v, a]).transpose()
        ekf['p_pre'] = np.eye(3)
        q_now = np.diag(q_init)
        r_now = np.diag(r_init)

    ### Time update
    u     = a - a_prev
    a_prev = a

    filter_type = 'ekf'
    if(filter_type == 'ekf'):
        # State vector
        ekf['x_pre'] = a_mat @ ekf['x_post'] + b * u
        # Error covariance matrix
        ekf['p_pre'] = a_mat @ (ekf['p_post'] @ a_mat.transpose()) + q_now

        ### Measurement update
        # Kalman Gain
        tmp = np.linalg.inv(np.matmul(np.matmul(h, ekf['p_pre']),h.transpose()) + r_now)
        ekf['k'] = np.matmul(np.matmul(ekf['p_pre'], h.transpose()), tmp)
        # state vector
        ekf['x_post'] = ekf['x_pre'] + np.matmul(ekf['k'], (y_m - np.matmul(h, ekf['x_pre'])))
        # Error covariance matrix
        ekf['p_post'] = np.matmul(np.eye(3) - np.matmul(ekf['k'], h), ekf['p_pre'])

        d_pre  = y_m - h @ ekf['x_pre']
        d_post = y_m - h @ ekf['x_post']

        # Adaptive Filtering
        q_tmp = ekf['k'] @ np.diag(np.power(d_pre, 2)) @ ekf['k'].transpose()
        r_tmp = np.diag(np.power(d_post, 2)) + h @ ekf['p_post'] @ h.transpose()
        q_now = (1-alpha) * q_now + alpha * 1.0 * q_tmp
        r_now = (1-alpha) * r_now + alpha * 1.0 * r_tmp
        # for idx in range(len(q_init)):
        #     q_now[idx][idx] = np.max([q_now[idx][idx], q_init[idx]*0.1])
        # for idx in range(len(r_init)):
        #     r_now[idx][idx] = np.max([r_now[idx][idx], r_init[idx]*0.1])
    elif(filter_type == 'ukf'):
        pass
        # length = 3
        # kappa = 1
        # alpha = 1
        # beta = 1
        # lamda = alpha * alpha * (length + kappa) - length
        # # eta_m = lamda / (length + lamda)
        # # eta_c = lamda / (length + lamda) + 1 - alpha * alpha + beta
        # eta_m = 1 / (2 * (length + lamda))


        # chi = a_mat @ chi
        # x_pre = eta_m * chi
        # p_pre = np.diag(eta_c * np.power(chi_x - x_pre,2))
        # tau = h @ chi
        # y_pre = eta_m * tau

    
    y_true = np.array([p, v, a]).transpose()
    d1_pre  = y_true -  ekf['x_pre']
    d1_post = y_true -  ekf['x_post']
    ekf['d1_s'].append(np.abs(d1_pre[0]))
    ekf['d1_v'].append(np.abs(d1_pre[1]))
    ekf['d1_a'].append(np.abs(d1_pre[2]))
    ekf['d2_s'].append(np.abs(d1_post[0]))
    ekf['d2_v'].append(np.abs(d1_post[1]))
    ekf['d2_a'].append(np.abs(d1_post[2]))

    # break
    
    ### Residual
    ekf['q_s'].append(q_now[0][0])
    ekf['q_v'].append(q_now[1][1])
    ekf['q_a'].append(q_now[2][2])
    ekf['r_s'].append(r_now[0][0])
    ekf['r_a'].append(r_now[1][1])

    ############################################
    ## Recording
    ############################################
    ekf['s_pre'].append(ekf['x_pre'][0])
    ekf['v_pre'].append(ekf['x_pre'][1])
    ekf['a_pre'].append(ekf['x_pre'][2])
    ekf['s_post'].append(ekf['x_post'][0])
    ekf['v_post'].append(ekf['x_post'][1])
    ekf['a_post'].append(ekf['x_post'][2])


plt.figure(1, figsize=[8, 8])
plt.title("tau = {}".format(tau))
ax = plt.subplot(331)
ax.plot(model['t'], model['s'])
ax.plot(ekf['t'], ekf['s_pre'], ":")
ax.plot(ekf['t'], ekf['s_post'],'--')
ax.plot(model['t'], model['s_true'],':r')
ax.legend(['True', 'Pre', 'Post'])
ax.set_ylabel('Position [m]'); ax.grid()
ax = plt.subplot(332)
ax.plot(ekf['t'], ekf['q_s'], ":")
ax.plot(ekf['t'], ekf['r_s'],'--')
ax.set_ylabel('Covariance [m]'); ax.grid()
ax = plt.subplot(333)
ax.plot(ekf['t'], ekf['d1_s'], ":")
ax.plot(ekf['t'], ekf['d2_s'],'--')
ax.set_ylabel('s_true - s_est [m]'); ax.grid()

ax = plt.subplot(334)
ax.plot(model['t'], model['v'])
ax.plot(ekf['t'], ekf['v_pre'], ":")
ax.plot(ekf['t'], ekf['v_post'],'--')
ax.legend(['True', 'Pre', 'Post'])
ax.set_ylabel('Velocity [m/s]'); ax.grid()
ax = plt.subplot(335)
ax.plot(ekf['t'], ekf['q_v'], ":")
ax.set_ylabel('Covariance [m]'); ax.grid()
ax = plt.subplot(336)
ax.plot(ekf['t'], ekf['d1_v'], ":")
ax.plot(ekf['t'], ekf['d2_v'],'--')
ax.set_ylabel('v_true - v_est [m]'); ax.grid()

ax = plt.subplot(337)
ax.plot(model['t'], model['a'])
ax.plot(ekf['t'], ekf['a_pre'], ":")
ax.plot(ekf['t'], ekf['a_post'],'--')
ax.plot(model['t'], model['a_true'],':r')
ax.legend(['True', 'Pre', 'Post'])
ax.set_ylabel('Acceleration [m/s2]'); ax.grid()
ax = plt.subplot(338)
ax.plot(ekf['t'], ekf['q_a'], ":")
ax.plot(ekf['t'], ekf['r_a'],'--')
ax.set_ylabel('Covariance [m]'); ax.grid()
ax = plt.subplot(339)
ax.plot(ekf['t'], ekf['d1_a'], ":")
ax.plot(ekf['t'], ekf['d2_a'],'--')
ax.set_ylabel('a_true - a_est [m/2]'); ax.grid()

plt.tight_layout()
plt.show()