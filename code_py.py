import serial
import pyautogui
import io

# Open Serial Port 11
ser = serial.Serial("COM11", 115200, timeout=0.05)
print('COM 11 Open:', ser.is_open)

sio = io.TextIOWrapper(io.BufferedRWPair(ser, ser))
ser.flush()

pyautogui.FAILSAFE = False
center = 0

while True:
    line = sio.readline().strip()
    if not line:
        continue
    
    data = line.split()

    # Check if the data has sufficient length
    if len(data) >= 3:
        x = float(data[1])
        y = float(data[0])
        third_value = int(data[2])
        
        # Check if the third value has changed
        if third_value != center:
            pyautogui.press('5')
            center = third_value

        # Limit the x and y values to the range [-90, 90]
        x = max(min(x, 50), -50)
        y = max(min(y, 50), -50)

        if -50 < x < -30:
            pyautogui.keyDown('left')
            pyautogui.keyUp('left')
        elif 30 < x < 50:
            pyautogui.keyDown('right')
            pyautogui.keyUp('right')
        if 30 < y < 50:
            pyautogui.keyDown('up')
            pyautogui.keyUp('up')
        elif -50 < y < -30:
            pyautogui.keyDown('down')
            pyautogui.keyUp('down')
    else:
        print("Data length insufficient")
