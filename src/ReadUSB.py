import re
import os
import time
import serial
import serial.tools.list_ports as port_list

def FindTeensy():
    # this method can return a list of ports
    for usb in port_list.comports():
        description = usb.description
        name = usb.name
        usbPort = usb.device
        return usbPort  # usbPort should be COM3 here (in Windows OS)

def CreateConnection():
    device = FindTeensy()
    # console_camera = serial.Serial(port=device, baudrate=1000000, , stopbits=1, bytesize=8, timeout=.1)
    console_camera = serial.Serial(port=device, baudrate=9600)

    return console_camera

def CheckConnection():
    if console_camera.is_open:
        print("Open")
    else:
        print("Closed")

def ReceiveData(Pin, console_camera, t=10):
    '''
    To collect data from Serial port and store it
    :param Pin: type:list  Pin is a list, containing all the series number of the pins
    :param console_camera: type:Serial camera
    :param t: type:int collecting times
    :return: null
    '''
    start = time.time()
    if os.path.exists('data.txt'):
        os.remove('data.txt')
    for pin in Pin:
        if os.path.exists('datalog_pin' + str(pin) + '.txt'):
            os.remove('datalog_pin' + str(pin) + '.txt')
        if os.path.exists('datalog_offset_pin' + str(pin) + '.txt'):
            os.remove('datalog_offset_pin' + str(pin) + '.txt')
        if os.path.exists('datalog_ave_pin' + str(pin) + '.txt'):
            os.remove('datalog_ave_pin' + str(pin) + '.txt')
        if os.path.exists('datalog_mid_pin' + str(pin) + '.txt'):
            os.remove('datalog_mid_pin' + str(pin) + '.txt')
        if os.path.exists('datalog_midave_pin' + str(pin) + '.txt'):
             os.remove('datalog_midave_pin' + str(pin) + '.txt')
    while(True):
        timeout = start + t
        time.sleep(0.5) # interval is 0.5s
        text = console_camera.read_all().decode('utf-8')
        if text == '':
            print("Empty Data")
        else:
            with open('data.txt', 'a+') as f:
                f.write(text)
                f.write('************************************************')
            for pin in Pin:
                value_touch, value_offset, value_ave, value_mid, value_midave = [], [], [], [], []
                print("Exacting data from touch sensors.")
                touchPin = re.findall("Pin+.+?:+.+?\r", text)
                offSetPin = re.findall("Offset+.+?:+.+?\r", text)
                avePin = re.findall("Average+.+?:+.+?\r", text)
                midPin = re.findall("Midian+.+?:+.+?\r", text)
                midavePin = re.findall("MidAve+.+?:+.+?\r", text)
                for each in touchPin:
                    if str(pin) in each:
                        result = re.findall(":(.+?\r)", each)[0]
                        result = result[:-2]
                        value_touch.append(result)

                for each in offSetPin:
                    if str(pin) in each:
                        result = re.findall(":(.+\r)", each)[0]
                        result = result[:-2]
                        value_offset.append(result)

                for each in avePin:
                    if str(pin) in each:
                        result = re.findall(":(.+\r)", each)[0]
                        result = result[:-2]
                        value_ave.append(result)

                for each in midPin:
                    if str(pin) in each:
                        result = re.findall(":(.+\r)", each)[0]
                        result = result[:-2]
                        value_mid.append(result)

                for each in midavePin:
                    if str(pin) in each:
                        result = re.findall(":(.+\r)", each)[0]
                        result = result[:-2]
                        value_midave.append(result)

                touch = ','.join(value_touch)
                offset = ','.join(value_offset)
                ave = ','.join(value_ave)
                mid = ','.join(value_mid)
                midave = ','.join(value_midave)
                with open('datalog_pin' + str(pin) + '.txt', 'a+') as f1: # add
                    f1.write(touch)
                    if touch:
                        f1.write(',')
                with open('datalog_offset_pin' + str(pin) + '.txt', 'a+') as f2:
                    f2.write(offset)
                    if offset:
                        f2.write(',')
                with open('datalog_ave_pin' + str(pin) + '.txt', 'a+') as f3:
                    f3.write(ave)
                    if ave:
                        f3.write(',')
                with open('datalog_mid_pin' + str(pin) + '.txt', 'a+') as f4:
                    f4.write(mid)
                    if mid:
                        f4.write(',')
                with open('datalog_midave_pin' + str(pin) + '.txt', 'a+') as f5:
                    f5.write(midave)
                    if midave:
                        f5.write(',')

        if time.time() > timeout:
            print("Ending")
            break

if __name__ == '__main__':
     console_camera = CreateConnection() # define a camera
     CheckConnection() # check the state
     Pin = [17]
     ReceiveData(Pin, console_camera, t=120)
     console_camera.close()
     CheckConnection()





