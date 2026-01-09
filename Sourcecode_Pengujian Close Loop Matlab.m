% === SETUP SERIAL ===
port = 'COM7';      % Ganti sesuai port Arduino
baud = 115200;
s = serial(port,'BaudRate',baud);
fopen(s);

disp('Mulai rekam... Tekan Ctrl+C untuk berhenti');

t = [];
suhu = [];
setp = [];

figure;
xlabel('Waktu (detik)');
ylabel('Suhu (°C)');
title('Uji PID Suhu Inkubator (Real-Time)');
grid on;
hold on;

% === Inisialisasi line handle ===
h_suhu = plot(NaN, NaN, 'b-', 'LineWidth', 1.5);
h_setp = plot(NaN, NaN, 'r--', 'LineWidth', 1.5);
legend('Suhu','Setpoint','Location','southwest');

% === Variabel tambahan untuk analisis ===
rise_time = NaN;
rise_time_done = false;

try
    while true
        % ==== Baca data serial dengan proteksi ====
        try
            data = fgetl(s);
        catch
            continue; % kalau gagal baca, skip loop
        end

        if isempty(data)
            continue; % kalau kosong, skip
        end

        nilai = str2double(strsplit(strtrim(data), ','));

        if length(nilai) == 3 && all(~isnan(nilai))
            t(end+1)    = nilai(1);
            suhu(end+1) = nilai(2);
            setp(end+1) = nilai(3);

            sp = setp(1); % Asumsi setpoint konstan
            y  = suhu;
            tt = t - t(1); % waktu mulai dari nol

            if length(y) > 10
                % ==== Rise Time (hitung sekali saja) ====
                if ~rise_time_done
                    if sp > y(1)   % kasus naik
                        y10 = y(1) + 0.1*(sp - y(1));
                        y90 = y(1) + 0.9*(sp - y(1));
                        t_rise_start = find(y >= y10, 1);
                        t_rise_end   = find(y >= y90, 1);
                    else           % kasus turun
                        y10 = y(1) - 0.1*(y(1) - sp);
                        y90 = y(1) - 0.9*(y(1) - sp);
                        t_rise_start = find(y <= y10, 1);
                        t_rise_end   = find(y <= y90, 1);
                    end

                    if ~isempty(t_rise_start) && ~isempty(t_rise_end)
                        rise_time = tt(t_rise_end) - tt(t_rise_start);
                        rise_time_done = true; % tandai sudah dihitung
                    end
                end

                % ==== Settling Time (2%) ====
                within_band = abs(y - sp) <= 0.02*abs(sp);
                if any(within_band)
                    last_idx = find(~within_band, 1, 'last');
                    if isempty(last_idx)
                        settling_time = tt(end);
                    elseif last_idx < length(tt)
                        settling_time = tt(last_idx+1);
                    else
                        settling_time = tt(end);
                    end
                else
                    settling_time = NaN;
                end

                % ==== Overshoot ====
                ymax = max(y);
                if sp ~= 0
                    max_overshoot = (ymax - sp) / abs(sp) * 100;
                else
                    max_overshoot = NaN;
                end

                % ==== Steady-State Error ====
                steady_state_error = abs(sp - y(end));

                % Tampilkan di Command Window
                fprintf('Rise Time: %.2f s | Settling Time: %.2f s | Overshoot: %.2f %% | SSE: %.2f °C\n', ...
                    rise_time, settling_time, max_overshoot, steady_state_error);
            else
                settling_time = NaN;
                max_overshoot = NaN;
                steady_state_error = NaN;
            end

            % === Update Grafik Real-Time ===
            set(h_suhu, 'XData', tt, 'YData', suhu);
            set(h_setp, 'XData', tt, 'YData', setp);

            % Info di grafik
            if ~isnan(rise_time)
                info_txt = {
                    ['Rise Time: ' num2str(rise_time,'%.2f') ' s'], ...
                    ['Settling Time: ' num2str(settling_time,'%.2f') ' s'], ...
                    ['Overshoot: ' num2str(max_overshoot,'%.2f') ' %'], ...
                    ['SSE: ' num2str(steady_state_error,'%.2f') ' °C']
                    };
                % posisi teks relatif
                x_pos = tt(end) - (tt(end)-tt(1))*0.4;
                y_pos = sp + 2;
                % hapus text lama
                delete(findall(gca,'Type','text'));
                text(x_pos, y_pos, info_txt, 'BackgroundColor','w','EdgeColor','k');
            end

            drawnow;
        end
    end
catch
    disp('Rekaman dihentikan');
end

% === Simpan data ===
data_tabel = table(t', suhu', setp', 'VariableNames', {'Waktu_dtk','Suhu','Setpoint'});
writetable(data_tabel, 'cc_c1.csv');
disp('Data berhasil disimpan di file: data_pid.csv');

fclose(s);
delete(s);
clear s;