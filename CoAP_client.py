import socket

from zeroconf import ServiceBrowser, ServiceListener, Zeroconf

from coapthon.client.helperclient import HelperClient
from coapthon.utils import parse_uri

__author__ = 'Giacomo Tanganelli'

ip = ""

class MyListener(ServiceListener):

    """def update_service(self, zc: Zeroconf, type_: str, name: str) -> None:
        print(f"Service {name} updated")

    def remove_service(self, zc: Zeroconf, type_: str, name: str) -> None:
        print(f"Service {name} removed")"""

    def add_service(self, zc: Zeroconf, type_: str, name: str) -> None:
        global ip

        info = zc.get_service_info(type_, name)
        print(f"Service {name} added, service info: {info}")
        ip = info.addresses.pop()
        ip = '.'.join(f'{c}' for c in ip)
        print("Local ip address found: " + ip)

def get_method(path):
    host, port, path = parse_uri(path)
    client = HelperClient(server=(host, port))
    try:
        tmp = socket.gethostbyname(host)
        host = tmp
    except socket.gaierror:
        pass

    response = client.get(path)
    print(response.pretty_print())
    client.stop()

def put_method(path, payload):
    payload = str(payload)
    host, port, path = parse_uri(path)
    client = HelperClient(server=(host, port))
    try:
        tmp = socket.gethostbyname(host)
        host = tmp
    except socket.gaierror:
        pass

    response = client.put(path, payload)
    print(response.pretty_print())
    client.stop()

def main():
    user_input = ""
    no_exit = True
    state_payload = 0
    monitor_payload = 0
    anemometer_unit_payload = 0
    prefix = "coap://"
    global ip
    port = "5683"
    #feedback_path = "coap://192.168.100.131:5683/feedback"
    feedback_path = "/feedback"
    pollution_path = "/pollution"
    radiation_path = "/radiation"
    precipitation_path = "/precipitation"
    wind_speed_path = "/wind_speed"
    state_path = "/state"
    monitor_path = "/monitor"
    anemometer_unit_path = "/anemometer_unit"

    # Get the local ESP32S3 ipv4 address
    zeroconf = Zeroconf()
    listener = MyListener()
    browser = ServiceBrowser(zeroconf, "_http._tcp.local.", listener)
    try:
        input("Press enter to exit...\n\n")
    finally:
        zeroconf.close()

    # Build the paths with the obtained ip
    feedback_path = prefix + ip + ":" + port + feedback_path
    pollution_path = prefix + ip + ":" + port + pollution_path
    radiation_path = prefix + ip + ":" + port + radiation_path
    precipitation_path = prefix + ip + ":" + port + precipitation_path
    wind_speed_path = prefix + ip + ":" + port + wind_speed_path
    state_path = prefix + ip + ":" + port + state_path
    monitor_path = prefix + ip + ":" + port + monitor_path
    anemometer_unit_path = prefix + ip + ":" + port + anemometer_unit_path

    while(no_exit):
        print("Choose an option:")
        print("1) Turn on/off monitor state.")
        print("2) Turn on/off local print result.")
        print("3) Change anemometer unit (Km/h or MPH).")
        print("4) Get feedback of monitor station.")
        print("5) Get pollution.")
        print("6) Get radiation.")
        print("7) Get precipitation.")
        print("8) Get wind speed.")
        print("9) Exit")
        print(">> ", end="")
        user_input = input()

        # PUT state
        if(user_input == "1"):
            state_payload = state_payload ^ 1
            put_method(state_path, state_payload)
        #PUT monitor
        elif(user_input == "2"):
            monitor_payload = monitor_payload ^ 1
            put_method(monitor_path, monitor_payload)
        # PUT anemometer unit
        elif(user_input == "3"):
            anemometer_unit_payload = anemometer_unit_payload ^ 1
            put_method(anemometer_unit_path, anemometer_unit_payload)
        # GET feedback
        elif(user_input == "4"):
            get_method(feedback_path)
        # GET pollution
        elif(user_input == "5"):
            get_method(pollution_path)
        # GET radiation
        elif(user_input == "6"):
            get_method(radiation_path)
        # GET precipitation
        elif(user_input == "7"):
            get_method(precipitation_path)
        # GET wind speed
        elif(user_input == "8"):
            get_method(wind_speed_path)
        # Exit
        elif(user_input == "9"):
            no_exit = False
        else:
            print("Error: Invalid option\n")

if __name__ == '__main__':
    main()
