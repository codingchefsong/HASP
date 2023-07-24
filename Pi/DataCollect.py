import serial
import time
from datetime import datetime
import threading
from serial.tools import list_ports
import logging
import smbus
import os

"""""""""""""""""""""""""""""""""""""""""""""
            Data collect
"""""""""""""""""""""""""""""""""""""""""""""

# Configuration
abs_path = "/home/shuyu/"

baud_rate = 9600  # Must match the baud rate set on the UART devices

# Configure the logging
log_file = abs_path + "application.log"
log_format = '%(asctime)s - %(levelname)s - {%(pathname)s:%(lineno)d} - %(message)s'
logging.basicConfig(filename=log_file, level=logging.INFO, format=log_format)

# Dictionary to store running threads and file names
running_threads = {}

"""""""""""""""""""""""""""""""""""""""""""""
            Sync with RTC clock
"""""""""""""""""""""""""""""""""""""""""""""

bus = smbus.SMBus(1)  # Create an I2C bus object (1 indicates the I2C bus number)
DS1307_ADDRESS = 0x68  # I2C address of the DS1307 RTC module


def bcd_to_decimal(bcd):
    return ((bcd // 0x10) * 10) + (bcd % 0x10)


def decimal_to_bcd(decimal):
    return ((decimal // 10) * 0x10) + (decimal % 10)


def read_datetime():
    # Read the date and time from the RTC
    rtc_data = bus.read_i2c_block_data(DS1307_ADDRESS, 0, 7)
    # print("Read:",rtc_data)
    second = bcd_to_decimal(rtc_data[0] & 0x7F)  # Mask out CH bit
    minute = bcd_to_decimal(rtc_data[1])
    hour = bcd_to_decimal(rtc_data[2] & 0x3F)  # Mask out 24-hour mode bit
    day = bcd_to_decimal(rtc_data[3])
    date = bcd_to_decimal(rtc_data[4])
    month = bcd_to_decimal(rtc_data[5])
    year = bcd_to_decimal(rtc_data[6]) + 2000
    # print("Output:",year,month,date,day,hour,minute,second)
    return datetime(year, month, date, hour, minute, second)


try:
    # Set the desired date and time
    desired_time = read_datetime()
    # Format the desired time as a string in the required format
    formatted_time = desired_time.strftime("%Y-%m-%d %H:%M:%S")
    # Set the system time using the 'date' command with sudo privileges
    os.system('sudo date -s "{}"'.format(formatted_time))
    logging.info(f"Time Sync successfully.")
except Exception as e:
    logging.error(f"Time Sync Failed.")
    logging.error(f"{str(e)}")
    os.system('sudo date -s \"2023-01-01 00:00:00\"')

"""""""""""""""""""""""""""""""""""""""""""""
            Data collect
"""""""""""""""""""""""""""""""""""""""""""""


# Function to read data from UART device and write to file
def read_and_write_data(uart_port, stop_event):
    uart = serial.Serial(uart_port, baud_rate)

    while not stop_event.is_set():
        try:
            if uart.in_waiting > 0:
                now = datetime.now()
                dt = now.strftime("%m/%d/%Y,%H:%M:%S.%f")
                data = f"#{uart_port},{dt},{uart.readline().decode('utf-8').rstrip()}\n"
                f_name = f"{abs_path}{datetime.now().strftime('%Y%m%d')}_{uart_port.replace('/', '')}.csv"
                # downlink msg
                data_dict = csv_to_dict(data)
                if data_dict:
                    downlink_msg.update(data_dict)
                # write to file
                with open(f_name, "a", encoding="utf-8") as f:
                    f.write(data)
            time.sleep(0.1)
        except Exception as e:
            logging.error(f"{uart_port} - {str(e)}")
            time.sleep(1)

    logging.info(f"Thread for UART port {uart_port} stopped.")
    print(f"Thread for UART port {uart_port} stopped.")


# Detect available UART ports
def detect_uart_ports():
    uart_ports = []
    ports = list(list_ports.comports())

    for port in ports:
        if "FT232R" in port.description:
            uart_ports.append(port.device)

    return uart_ports


"""""""""""""""""""""""""""""""""""""""""""""
            Downlink Data
"""""""""""""""""""""""""""""""""""""""""""""


def csv_to_dict(csv_data):
    # Split the CSV string into key-value pairs
    result = {}
    pairs = csv_data.split(',')
    if len(pairs) > 20:
        result = {pairs[4]: {pairs[i]: pairs[i + 1] for i in range(3, len(pairs) - 1, 2)}}
    return result


def send_downlink(stop_event):
    while not stop_event.is_set():
        try:
            dt = datetime.now().strftime("%H:%M:%S")
            msg = "CU" + dt
            sorted_dict = dict(sorted(downlink_msg.items()))

            for k, v in sorted_dict.items():
                print(k,v)
                if "TempC" in v:
                    msg += str(k + v["TempC"])
                else:
                    msg += str(k)
            msg += '\n'

            downlink_msg.clear()
            # print(sorted_dict)
            print("Downlink send: ", msg)
            with serial.Serial('/dev/ttyS0', 1200, timeout=1) as ser:
                ser.write(msg.encode())
            # ser.write((str(downlink_msg)+'\n').encode())  # Send the message via serial

            time.sleep(2)  # Delay for 1 second before sending the next message
        except Exception as e:
            logging.error(f"Downlink - {str(e)}")
            time.sleep(1)


"""""""""""""""""""""""""""""""""""""""""""""
            Variables
"""""""""""""""""""""""""""""""""""""""""""""

# Create and start threads for each UART device
threads = []
stop_event = threading.Event()
downlink_msg = {}

thread_stop_event = threading.Event()
thread = threading.Thread(target=send_downlink,
                          args=(thread_stop_event,))
thread.start()
threads.append(thread)
running_threads["/dev/ttyS0"] = (thread, thread_stop_event)

while True:
    uart_ports = detect_uart_ports()

    # Check if new UART ports are detected
    new_ports = set(uart_ports) - set(running_threads.keys())
    for new_port in new_ports:
        thread_stop_event = threading.Event()
        thread = threading.Thread(target=read_and_write_data,
                                  args=(new_port, thread_stop_event))
        thread.start()
        threads.append(thread)
        running_threads[new_port] = (thread, thread_stop_event)
        logging.critical(f"UART port {new_port} connected")
        print(f"UART port {new_port} connected")
        logging.info(f"Thread for UART port {new_port} started.")
        print(f"Thread for UART port {new_port} started.")

    # Check if any UART ports are disconnected
    disconnected_ports = set(running_threads.keys()) - set(uart_ports)
    for disconnected_port in disconnected_ports:
        if disconnected_port == "/dev/ttyS0":
            pass
        else:
            logging.critical(f"UART port {disconnected_port} disconnected")
            print(f"UART port {disconnected_port} disconnected")
            thread, thread_stop_event = running_threads[disconnected_port]
            thread_stop_event.set()  # Set the stop event to stop the thread
            thread.join()  # Wait for the thread to finish
            del running_threads[disconnected_port]

    time.sleep(1)  # Wait for 1 second before checking again
