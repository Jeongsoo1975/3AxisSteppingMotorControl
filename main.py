import tkinter as tk
from tkinter import ttk, messagebox
import serial
import serial.tools.list_ports
import threading
import time
import os
import configparser


class InputValidator:
    """
    Entry 위젯의 입력을 실시간으로 검증하는 클래스.
    입력값이 숫자인지, 그리고 지정된 최댓값보다 작은지 확인합니다.
    """

    def __init__(self, max_entry_widget):
        self.max_entry = max_entry_widget

    def validate(self, P):
        """입력값(P)을 검증하는 콜백 함수."""
        if P == "" or P == "-":
            return True

        try:
            max_val_str = self.max_entry.get()
            max_val = int(max_val_str) if max_val_str else 10000

            value = int(P)

            if value > max_val:
                return False
        except ValueError:
            return False

        return True


class CncControllerApp:
    def __init__(self, root):
        self.root = root
        self.root.title("3-Axis Stepper Motor Controller")
        self.root.geometry("540x550")

        self.serial_port = None
        self.connected = False
        self.is_reading = False
        self.settings_file = "settings.txt"  # --- [수정] 설정 파일 이름

        # --- UI 프레임 구성 ---
        connection_frame = ttk.LabelFrame(root, text="Connection")
        connection_frame.pack(padx=10, pady=10, fill="x")

        self.notebook = ttk.Notebook(root)
        self.notebook.pack(padx=10, pady=5, expand=True, fill="both")

        normal_mode_frame = ttk.Frame(self.notebook, padding=10)
        test_mode_frame = ttk.Frame(self.notebook, padding=10)

        self.notebook.add(normal_mode_frame, text="Normal Mode")
        self.notebook.add(test_mode_frame, text="Test Mode")

        status_frame = ttk.LabelFrame(root, text="Arduino Status")
        status_frame.pack(padx=10, pady=(0, 10), fill="both", expand=True)

        # --- 위젯 구성 ---

        # 1. 연결 프레임 위젯
        ttk.Label(connection_frame, text="COM Port:").grid(row=0, column=0, padx=5, pady=5, sticky="w")
        self.port_combobox = ttk.Combobox(connection_frame, width=12)
        self.port_combobox.grid(row=0, column=1, padx=5, pady=5)
        self.connect_button = ttk.Button(connection_frame, text="Connect", command=self.toggle_connection)
        self.connect_button.grid(row=0, column=2, padx=5, pady=5)
        self.refresh_button = ttk.Button(connection_frame, text="Refresh", command=self.update_ports)
        self.refresh_button.grid(row=0, column=3, padx=5, pady=5)
        self.led_canvas = tk.Canvas(connection_frame, width=22, height=22, bd=0, highlightthickness=0)
        self.led_canvas.grid(row=0, column=4, padx=(10, 5), pady=5)
        self.led_indicator = self.led_canvas.create_oval(3, 3, 20, 20, fill='red', outline='gray')
        ttk.Label(connection_frame, text="Status").grid(row=0, column=5, padx=(0, 5), pady=5, sticky="w")

        # 2. 일반 모드 프레임 위젯 (레이아웃 수정)
        move_controls_frame = ttk.LabelFrame(normal_mode_frame, text="Axis Controls")
        move_controls_frame.pack(fill="x", expand=True)

        ttk.Label(move_controls_frame, text="Position", font=('Helvetica', 9, 'bold')).grid(row=0, column=1, padx=5,
                                                                                            pady=2)
        ttk.Label(move_controls_frame, text="Max Value", font=('Helvetica', 9, 'bold')).grid(row=0, column=2, padx=5,
                                                                                             pady=2)

        self.x_entry = ttk.Entry(move_controls_frame, width=10)
        self.x_max_entry = ttk.Entry(move_controls_frame, width=10)
        self.y_entry = ttk.Entry(move_controls_frame, width=10)
        self.y_max_entry = ttk.Entry(move_controls_frame, width=10)
        self.z_entry = ttk.Entry(move_controls_frame, width=10)
        self.z_max_entry = ttk.Entry(move_controls_frame, width=10)

        ttk.Label(move_controls_frame, text="X-Axis:").grid(row=1, column=0, padx=5, pady=8, sticky="w")
        self.x_entry.grid(row=1, column=1, padx=5, pady=8)
        self.x_max_entry.grid(row=1, column=2, padx=5, pady=8)
        self.move_x_button = ttk.Button(move_controls_frame, text="Move X", command=self.send_x_command)
        self.move_x_button.grid(row=1, column=3, padx=(5, 20), pady=8)

        ttk.Label(move_controls_frame, text="Y-Axis:").grid(row=2, column=0, padx=5, pady=8, sticky="w")
        self.y_entry.grid(row=2, column=1, padx=5, pady=8)
        self.y_max_entry.grid(row=2, column=2, padx=5, pady=8)
        self.move_y_button = ttk.Button(move_controls_frame, text="Move Y", command=self.send_y_command)
        self.move_y_button.grid(row=2, column=3, padx=(5, 20), pady=8)

        ttk.Label(move_controls_frame, text="Z-Axis:").grid(row=3, column=0, padx=5, pady=8, sticky="w")
        self.z_entry.grid(row=3, column=1, padx=5, pady=8)
        self.z_max_entry.grid(row=3, column=2, padx=5, pady=8)
        self.move_z_button = ttk.Button(move_controls_frame, text="Move Z", command=self.send_z_command)
        self.move_z_button.grid(row=3, column=3, padx=(5, 20), pady=8)

        vcmd_x = (root.register(InputValidator(self.x_max_entry).validate), '%P')
        vcmd_y = (root.register(InputValidator(self.y_max_entry).validate), '%P')
        vcmd_z = (root.register(InputValidator(self.z_max_entry).validate), '%P')

        self.x_entry.config(validate="key", validatecommand=vcmd_x)
        self.y_entry.config(validate="key", validatecommand=vcmd_y)
        self.z_entry.config(validate="key", validatecommand=vcmd_z)

        system_controls_frame = ttk.LabelFrame(normal_mode_frame, text="System & Recovery")
        system_controls_frame.pack(fill="x", expand=True, pady=(10, 0))

        self.move_all_button = ttk.Button(system_controls_frame, text="Move All Axes",
                                          command=self.send_move_all_command)
        self.move_all_button.pack(side=tk.LEFT, padx=5, pady=5, expand=True, fill="x")
        self.home_button = ttk.Button(system_controls_frame, text="Homing", command=self.send_home_command)
        self.home_button.pack(side=tk.LEFT, padx=5, pady=5, expand=True, fill="x")
        self.reverse_direction_var = tk.BooleanVar()
        style = ttk.Style(self.root)
        style.configure("Switch.TCheckbutton", font=('Helvetica', 10))
        self.reverse_toggle = ttk.Checkbutton(system_controls_frame, text="Reverse Direction",
                                              variable=self.reverse_direction_var, style="Switch.TCheckbutton")
        self.reverse_toggle.pack(side=tk.LEFT, padx=5, pady=5, expand=True, fill="x")

        # 테스트 모드 위젯들
        ttk.Label(test_mode_frame, text="Move X-Axis to:").grid(row=0, column=0, padx=5, pady=8, sticky="w")
        self.x_test_entry = ttk.Entry(test_mode_frame, width=10);
        self.x_test_entry.grid(row=0, column=1, padx=5, pady=8);
        self.x_test_entry.insert(0, "100")
        self.x_test_button = ttk.Button(test_mode_frame, text="Move X (Test)", command=self.send_x_test_command);
        self.x_test_button.grid(row=0, column=2, padx=5, pady=8)

        self.status_text = tk.Text(status_frame, height=10, state="disabled", wrap="word")
        self.status_text.pack(padx=5, pady=5, fill="both", expand=True)

        self.root.protocol("WM_DELETE_WINDOW", self.on_closing)
        self.update_led('disconnected')
        self.update_ports()
        self.load_settings()  # --- [수정] 프로그램 시작 시 설정 불러오기

    # --- [신규] 설정 저장 함수 ---
    def save_settings(self):
        config = configparser.ConfigParser()
        config['Settings'] = {
            'com_port': self.port_combobox.get(),
            'x_max': self.x_max_entry.get(),
            'y_max': self.y_max_entry.get(),
            'z_max': self.z_max_entry.get()
        }
        try:
            with open(self.settings_file, 'w') as configfile:
                config.write(configfile)
            self.update_status("Settings saved.\n")
        except Exception as e:
            self.update_status(f"Error saving settings: {e}\n")

    # --- [신규] 설정 불러오기 함수 ---
    def load_settings(self):
        config = configparser.ConfigParser()
        if not os.path.exists(self.settings_file):
            self.update_status("Settings file not found. Using default values.\n")
            # Max 값 기본값 설정
            self.x_max_entry.insert(0, "10000")
            self.y_max_entry.insert(0, "10000")
            self.z_max_entry.insert(0, "10000")
            return

        try:
            config.read(self.settings_file)
            settings = config['Settings']

            saved_port = settings.get('com_port')
            x_max = settings.get('x_max', '10000')
            y_max = settings.get('y_max', '10000')
            z_max = settings.get('z_max', '10000')

            # Max 값 설정
            self.x_max_entry.insert(0, x_max)
            self.y_max_entry.insert(0, y_max)
            self.z_max_entry.insert(0, z_max)

            # COM 포트 설정 및 자동 연결 시도
            if saved_port and saved_port in self.ports:
                self.port_combobox.set(saved_port)
                self.update_status(f"Loaded settings. Found saved port [{saved_port}]. Attempting to auto-connect...\n")
                self.toggle_connection()
            else:
                self.update_status(f"Loaded settings. Saved port [{saved_port}] not available.\n")
        except Exception as e:
            self.update_status(f"Error loading settings: {e}\n")

    # --- [수정] on_closing 함수에서 save_settings 호출 ---
    def on_closing(self):
        self.save_settings()
        if self.connected:
            self.toggle_connection()
        self.root.destroy()

    def update_led(self, status):
        color = 'green' if status == 'connected' else 'red'
        self.led_canvas.itemconfig(self.led_indicator, fill=color)

    def update_ports(self):
        self.ports = [port.device for port in serial.tools.list_ports.comports()]
        self.port_combobox['values'] = self.ports
        if self.ports: self.port_combobox.set(self.ports[0])

    def toggle_connection(self):
        if self.connected:
            self.is_reading = False
            if self.serial_port and self.serial_port.is_open: self.serial_port.close()
            self.connected = False;
            self.connect_button.config(text="Connect");
            self.update_status("Disconnected.\n");
            self.update_led('disconnected')
        else:
            port_name = self.port_combobox.get()
            if not port_name: messagebox.showerror("Error", "No COM port selected."); return
            try:
                self.serial_port = serial.Serial(port_name, 9600, timeout=1);
                time.sleep(2);
                self.connected = True
                self.connect_button.config(text="Disconnect");
                self.update_status(f"Successfully connected to {port_name}\n");
                self.update_led('connected')
                self.is_reading = True;
                self.read_thread = threading.Thread(target=self.read_from_serial, daemon=True);
                self.read_thread.start()
            except serial.SerialException as e:
                messagebox.showerror("Connection Error", f"Failed to connect to {port_name}:\n{e}")
                self.update_status(f"Failed to connect. Please check the device and select the correct port.\n")
                self.connected = False;
                self.connect_button.config(text="Connect");
                self.update_led('disconnected')

    def read_from_serial(self):
        while self.is_reading and self.serial_port.is_open:
            try:
                line = self.serial_port.readline().decode('utf-8').strip()
                if line: self.update_status(line + "\n")
            except (serial.SerialException, UnicodeDecodeError):
                break

    def send_command(self, command):
        if not self.connected: messagebox.showwarning("Warning", "Not connected to Arduino."); return
        try:
            full_command = f"<{command}>"
            self.serial_port.write(full_command.encode('utf-8'))
            self.update_status(f"Sent: {full_command}\n")
        except serial.SerialException as e:
            messagebox.showerror("Error", f"Failed to send command: {e}"); self.toggle_connection()

    def get_validated_value(self, pos_entry, max_entry):
        try:
            pos_str = pos_entry.get();
            pos_val = int(pos_str) if pos_str else 0
            max_str = max_entry.get();
            max_val = int(max_str) if max_str else 10000
            if abs(pos_val) > max_val:
                messagebox.showerror("Input Error", f"Position value ({pos_val}) cannot exceed Max value ({max_val}).")
                return None
            multiplier = -1 if self.reverse_direction_var.get() else 1
            return pos_val * multiplier
        except ValueError:
            messagebox.showerror("Input Error", "Position and Max values must be valid integers.")
            return None

    def send_home_command(self):
        if self.reverse_direction_var.get():
            self.send_command("HOME_REVERSED")
        else:
            self.send_command("HOME")

    def send_move_all_command(self):
        x = self.get_validated_value(self.x_entry, self.x_max_entry)
        y = self.get_validated_value(self.y_entry, self.y_max_entry)
        z = self.get_validated_value(self.z_entry, self.z_max_entry)
        if x is not None and y is not None and z is not None: self.send_command(f"X{x},Y{y},Z{z}")

    def send_x_command(self):
        val = self.get_validated_value(self.x_entry, self.x_max_entry)
        if val is not None: self.send_command(f"X{val}")

    def send_y_command(self):
        val = self.get_validated_value(self.y_entry, self.y_max_entry)
        if val is not None: self.send_command(f"Y{val}")

    def send_z_command(self):
        val = self.get_validated_value(self.z_entry, self.z_max_entry)
        if val is not None: self.send_command(f"Z{val}")

    def send_x_test_command(self):
        try:
            multiplier = -1 if self.reverse_direction_var.get() else 1
            value = int(self.x_test_entry.get()) * multiplier
            self.send_command(f"X{value}")
        except ValueError:
            messagebox.showerror("Invalid Input", "Please enter a valid integer for X in Test Mode.")

    def send_y_test_command(self):
        pass

    def send_z_test_command(self):
        pass

    def update_status(self, message):
        self.status_text.config(state="normal");
        self.status_text.insert(tk.END, message)
        self.status_text.see(tk.END);
        self.status_text.config(state="disabled")


if __name__ == "__main__":
    root = tk.Tk()
    app = CncControllerApp(root)
    root.mainloop()
