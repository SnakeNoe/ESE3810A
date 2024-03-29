import socket

from coapthon.client.helperclient import HelperClient
from coapthon.utils import parse_uri

__author__ = 'Giacomo Tanganelli'

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
    example_path = "coap://192.168.100.131:5683/Espressif"
    feedback_path = "coap://192.168.100.131:5683/feedback"
    pollution_path = "coap://192.168.100.131:5683/pollution"
    radiation_path = "coap://192.168.100.131:5683/radiation"
    precipitation_path = "coap://192.168.100.131:5683/precipitation"
    wind_speed_path = "coap://192.168.100.131:5683/wind_speed"
    state_path = "coap://192.168.100.131:5683/state"
    monitor_path = "coap://192.168.100.131:5683/monitor"
    anemometer_unit_path = "coap://192.168.100.131:5683/anemometer_unit"

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
