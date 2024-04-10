% Author: Tim Ausec
% Date: 3/25/2024
% Description:
% This script models a first order approximation for temperature with a
% simple feedback control loop.
clear variables
density=1.204; %kg/m^3
volume=(10^-2)^3; %m^3
m = density*volume; %mass
Cp = 1e3; % heat capacity: J/kg*K
Tf = 10; % seconds
t = linspace(0, Tf, 1000); % seconds
r = 0.5*ones(1, length(t)); % reference in Watts
r(1:length(t)/2)=ones(1,length(t)/2);
Kt = 25.72e-3; %thermal conductivity: W/m*K
A = 6*(10e-2)^2;
L = 2e-2;
Tinit=25+273; % kelvin

Gp = tf(1, [m*Cp Kt*A/L]);

% plot
kp=1;
kd=1;
ki=1;
Gc=tf([kd kp],1);

xt = [ t' r' ];
[num_Gp, den_Gp] = tfdata(Gp, 'v');
[num_Gc, den_Gc] = tfdata(Gc, 'v');

sim('Heater_Model.slx');

yout=ys+Tinit-273; % Celsius
plot(t,r,ts,yout)
xlabel("Time (s)")
ylabel("Temperature (C)")
title("Temperature over time: k_d= "+ num2str(kd)+ " k_i= "+ num2str(ki)+"k_p= "+ num2str(kp))
