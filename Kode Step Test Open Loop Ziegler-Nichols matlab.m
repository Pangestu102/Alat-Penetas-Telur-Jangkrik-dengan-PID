% === SETUP SERIAL ===
port = 'COM7';   % Ganti sesuai port Arduino
baud = 115200;
s = serial(port,'BaudRate',baud);
fopen(s);

max_time = 2400;  % durasi perekaman = 2000 detik
data = [];
time = [];

figure;
title('Step Response');
xlabel('Waktu (detik)');
ylabel('Suhu (°C)');
grid on;
hold on;

disp('Mulai rekam data... Tekan Ctrl+C untuk berhenti');

tic;
while toc < max_time
    if s.BytesAvailable > 0
        suhu = str2double(fgetl(s));
        t = toc;
        data = [data; suhu];
        time = [time; t];
        plot(time, data, 'b-');
        drawnow;
    end
end

fclose(s);

% === Simpan Data ===
T = table(time, data);
writetable(T, 'step_test_data.csv');
disp('Data disimpan ke step_test_data.csv');

%% === Hitung K, L, T ===
y0 = data(1);
y_ss = mean(data(end-10:end));
delta_y = y_ss - y0;
delta_u = 30;  % step input = 30%
K = delta_y / delta_u;

% Dead time L (5%)
idx_L = find(data > y0 + 0.05*delta_y, 1);
L = time(idx_L);

% Time constant T (63.2%)
idx_T = find(data > y0 + 0.632*delta_y, 1);
T_const = time(idx_T) - L;

fprintf('K = %.3f\n', K);
fprintf('L = %.3f detik\n', L);
fprintf('T = %.3f detik\n', T_const);

%% === Rumus Ziegler–Nichols Open Loop ===
% ZN untuk PID open loop:
% Kp = 1.2 * (T / (K * L))
% Ti = 2 * L
% Td = 0.5 * L

Kp = 1.2 * (T_const / (K * L));
Ti = 2 * L;
Td = 0.5 * L;

Ki = Kp / Ti;
Kd = Kp * Td;

fprintf('\n=== Ziegler–Nichols Open Loop PID ===\n');
fprintf('Kp = %.3f\n', Kp);
fprintf('Ki = %.3f\n', Ki);
fprintf('Kd = %.3f\n', Kd);
