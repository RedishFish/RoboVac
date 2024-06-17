'''
Author:     Philip Xu
Date:   2024/06/06
Description:    This GUI app offers Bluetooth control for the RoboVac. 
                It allows the user to control the motors and the fan. 
                It also allows the user to read the battery status.
'''


import customtkinter
import tkinter
import socket
import time

customtkinter.set_appearance_mode("dark")
customtkinter.set_default_color_theme("dark-blue")

class App(customtkinter.CTk):
    def __init__(self):
        super().__init__()

        ### Try connecting to Bluetooth ###
        try:
            self.BTConnection()
            print("connection successful")
        except:
            raise ConnectionError("Connection unsuccessful to robot. Check if the HC-05 Bluetooth device has been paired")

        ### GUI frame configs ###
        self.title("RoboVac")
        self.geometry(f"{700}x{1000}")

        ### Creating GUI components ###
        self.mainFrame = customtkinter.CTkFrame(self)
        self.mainFrame.pack(pady=20, padx=50, fill="both", expand=True)

        self.appTitle = customtkinter.CTkLabel(self.mainFrame, text="RoboVac User Interface", font=customtkinter.CTkFont(size=25, weight="bold"))
        self.appTitle.pack(pady=(20, 10))
        
        self.reconnectButton = customtkinter.CTkButton(self.mainFrame, text="Reconnect", command=self.BTConnection)
        self.reconnectButton.pack()
        
        # Frame containing the manual/auto mode buttons
        self.modeFrame = customtkinter.CTkFrame(self.mainFrame)
        self.modeFrame.pack(pady=20, padx=20, fill="both", expand=True)
        self.modeFrame.columnconfigure((0,1,2), weight=1)
        self.modeTitle = customtkinter.CTkLabel(self.modeFrame, text="Select Mode", font=customtkinter.CTkFont(size=20, weight="bold"))
        self.modeTitle.grid(pady=15, row=0, column=0)
        self.modeVar = tkinter.IntVar(value=0)
        self.autoBtn = customtkinter.CTkRadioButton(self.modeFrame, text="Auto", command=self.disableFeatures, variable=self.modeVar, value=0)
        self.autoBtn.grid(row=0, column=1, pady=10, padx=20)
        self.manualBtn = customtkinter.CTkRadioButton(self.modeFrame, text="Manual", command=self.enableFeatures ,variable=self.modeVar, value=1)
        self.manualBtn.grid(row=0, column=2, pady=10, padx=20)

        # Frame for motor control
        self.motorFrame = customtkinter.CTkFrame(self.mainFrame)
        self.motorFrame.pack(pady=20, padx=20, fill="both", expand=True)
        self.motorFrame.grid_columnconfigure(0, weight=1)
        self.motorTitle = customtkinter.CTkLabel(self.motorFrame, text="Motor Control", font=customtkinter.CTkFont(size=20, weight="bold"))
        self.motorTitle.grid(pady=15, row=0, column=0, columnspan=2)
        self.directionFrame =  customtkinter.CTkFrame(self.motorFrame)
        self.directionFrame.grid(pady=10, padx=(20, 0), row=1, column=0, rowspan=2, sticky="nsew")
        self.directionFrame.columnconfigure((0,1,2), weight=1)
        self.directionFrame.rowconfigure((0,1,2), weight=1)
        self.forwardBtn = customtkinter.CTkButton(self.directionFrame, text="Fwd", command=lambda p="forwardBtn":self.onMotorCommand(p))
        self.forwardBtn.grid(padx=5, pady=5, row=0, column=1, sticky="nsew")
        self.rightBtn = customtkinter.CTkButton(self.directionFrame, text="Right", command=lambda p="rightBtn":self.onMotorCommand(p))
        self.rightBtn.grid(padx=5, pady=5, row=1, column=2, sticky="nsew")
        self.leftBtn = customtkinter.CTkButton(self.directionFrame, text="Left", command=lambda p="leftBtn":self.onMotorCommand(p))
        self.leftBtn.grid(padx=5, pady=5, row=1, column=0, sticky="nsew")
        self.backwardBtn = customtkinter.CTkButton(self.directionFrame, text="Bwd", command=lambda p="backwardBtn":self.onMotorCommand(p))
        self.backwardBtn.grid(padx=5, pady=5, row=2, column=1, sticky="nsew")
        self.brakeBtn = customtkinter.CTkButton(self.directionFrame, text="Brake", command=lambda p="brakeBtn":self.onMotorCommand(p))
        self.brakeBtn.grid(padx=5, pady=5, row=1, column=1, sticky="nsew")
        self.motorFrame.grid_columnconfigure(1, weight=1)
        self.motorSpeedSlider = customtkinter.CTkSlider(self.motorFrame, orientation="vertical", command=self.onMotorSlider)
        self.motorSpeedSlider.grid(padx=20, pady=10, row=2, column=1)
        self.motorSliderLabel = customtkinter.CTkLabel(self.motorFrame, text="Speed")
        self.motorSliderLabel.grid(padx=20, pady=5, row=1, column=1)

        # Frame for other features: fan PWM control and battery reading
        self.othersFrame = customtkinter.CTkFrame(self.mainFrame)
        self.othersFrame.pack(pady=20, padx=20, fill="both", expand=True)
        self.othersFrame.grid_columnconfigure((1,4), weight=1)
        self.othersFrame.grid_columnconfigure((0,3), weight=3)
        self.othersFrame.grid_columnconfigure(2, weight=55)
        self.fanSliderLabel = customtkinter.CTkLabel(self.othersFrame, text="Fan speed")
        self.fanSliderLabel.grid(padx=20, pady=20, row=0, column=0, rowspan=2)
        self.fanSpeedSlider = customtkinter.CTkSlider(self.othersFrame, orientation="vertical", command=self.onFanSlider)
        self.fanSpeedSlider.grid(padx=20, pady=20, row=0, column=1, rowspan=2)
        self.powerLabel = customtkinter.CTkLabel(self.othersFrame, text="Power left ()")
        self.powerLabel.grid(padx=20, pady=0, row=0, column=3)
        self.powerBar = customtkinter.CTkProgressBar(self.othersFrame, orientation="vertical")
        self.powerBar.grid(padx=20, pady=20, row=0, column=4, rowspan=2)
        self.setPowerButton = customtkinter.CTkButton(self.othersFrame, text="Get", command=self.setBatteryStatus)
        self.setPowerButton.grid(padx=20, pady=0, row=1, column=3)

        # Disable features initially
        self.widgets = [self.forwardBtn, self.rightBtn, self.leftBtn, self.backwardBtn, self.brakeBtn, self.motorSpeedSlider, self.fanSpeedSlider, self.setPowerButton]
        self.disableFeatures()

        self.lastMillis1 = time.time()
        self.lastMillis2 = time.time()

    # Disable all interactable widgets
    def disableFeatures(self):
        for widget in self.widgets:
            widget.configure(state="disabled")
        try:
            self.client.send("mode: auto/".encode("utf-8"))
        except:
            pass

    # Enable all interactable widgets
    def enableFeatures(self):
        for widget in self.widgets:
            widget.configure(state="normal")
        self.client.send("mode: manual/".encode("utf-8"))

    # When a motor control button is pressed
    def onMotorCommand(self, parent):
        if parent == "forwardBtn":
            self.client.send("forward/".encode("utf-8"))
        elif parent == "leftBtn":
            self.client.send("turn-left/".encode("utf-8"))
        elif parent == "rightBtn":
            self.client.send("turn-right/".encode("utf-8"))
        elif parent == "backwardBtn":
            self.client.send("backward/".encode("utf-8"))
        elif parent == "brakeBtn":
            self.client.send("brake/".encode("utf-8"))
    
    # When the motor speed slider is changed
    def onMotorSlider(self, value):
        if time.time() - self.lastMillis1 > 0.250: # do not want to overwhelm the Arduino Serial port
            self.client.send(f"motor-speed-multiplier: {value+0.5}/".encode("utf-8"))
            print(f"Sent {value+0.5}")
            self.lastMillis1 = time.time()

    # When the fan slider is changed
    def onFanSlider(self, value):
        if time.time() - self.lastMillis2 > 0.250: # do not want to overwhelm the Arduino Serial port
            self.client.send(f"fan-speed-multiplier: {value}/".encode("utf-8"))
            self.lastMillis2 = time.time()
       
    # Create a new Bluetooth connection
    def BTConnection(self):
        self.client = socket.socket(socket.AF_BLUETOOTH, socket.SOCK_STREAM, socket.BTPROTO_RFCOMM)
        self.client.connect(("00:23:00:00:91:ef", 1))

    # Gets battery percentage from the robot, then displays it
    def setBatteryStatus(self):
        data = self.client.recv(1024).decode('utf-8').split("\r\n")
        for line in data:
            if line and line[-1] == "/" and line.startswith("battery: "):
                percent = float(line[9:-1])
                self.powerBar.set(percent/100)
                self.powerLabel.configure(text=f"Power left: ({percent}% of 12V) (67% is considered dead)")
                break


# Where the code is executed
if __name__ == "__main__":
    app = App()
    app.mainloop()
