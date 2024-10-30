#
# start with python -m flask --app aggregator.py run -p 5050 -h 0.0.0.0
#
import paho.mqtt.client as mqtt
import jsonpickle
from weight_dto import Weight_DTO
from flask import Flask, make_response, request
import requests
from threading import Thread
from markupsafe import escape
import struct
import re
import time

app = Flask(__name__)
ipAddressMQTT = "ipAddressMQTT"
portMQTT = portMQTT

sensorData = {}

# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))
    client.subscribe("cocktail/weight/+")
    client.subscribe("cocktail/pumpen/+")

# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
    sensor = msg.topic[msg.topic.rfind('/')+1:]
    weight = struct.unpack('<i', msg.payload)
    print(f"Received payload weight: {weight[0]}")
    if(sensor in sensorData.keys()):
        sensorData[sensor].weight = weight[0]
    else:
        sensorData[sensor] = Weight_DTO(sensor, weight[0])

# This code has to be put here to make it run with the flask run command
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message
client.connect(ipAddressMQTT, portMQTT, 60)
app.debug=True
client.loop_start()

@app.route("/", methods=['GET'])
def home():
    endpoints = {
        "GET" : {
            "weights": "Data of all sensors that data was aggregated on",
            "weights/<sensor-id>": "Data of a particular sensor specified by the <sensor-id> can be seen under the name property of the weight endpoint",
            "weights/<sensor-id>/name": "Name of the sensor, corresponds with <sensor-id>",
            "weights/<sensor-id>/weight": "Latest published weight of the sensor",
            "weights/<sensor-id>/full": "Weight of the completely filled container on the scale. Null if weight not set previously via a POST to this endpoint",
            "weights/<sensor-id>/empty": "Weight of the completely empty container on the scale. Null if weight not set previously via a POST to this endpoint",
            "weights/<sensor-id>/level": "Current fill level based on weight of the container on the scale, requires full and empty to be set!",
            "weights/tare": "Sends a tare command to all sensors to the current zero level."
        },
        "POST" : {
            "weights/<sensor-id>/full": "Sets the full value to the currently published weight. Used to calibrate the weight of a completely filled container on the scale.",
            "weights/<sensor-id>/empty": "Sets the empty value to the currently published weight. Used to calibrate the weight of a completely empty container on the scale.",
            "pumps/<sensor-id>/burst": "Starts the pump related to the given sensor for 1 sec.",
            "pumps/<sensor-id>/timed/<time>": "Starts the pump related to the given sensor for a time given in seconds.",
            "pumps/fill/<sensor-id>/<amount>": "Fills the glass with the liquid on the given weightsensor. The amount is in grams."
        }
        }
    return endpoints

@app.route("/weights", methods=['GET'])
def getAllWeightData():
    sensorDataJson = jsonpickle.encode(sensorData, unpicklable=False)
    response = make_response(sensorDataJson)
    response.content_type = 'application/json'
    return response

@app.route("/weights/<string:sensor>", methods=['GET'])
def getSensorData(sensor):
    sensor = escape(sensor)
    if sensor not in sensorData.keys():
        response = make_response('The requested sensor is not registered.', 404)
        return response
    sensorDataJson = jsonpickle.encode(sensorData.get(sensor), unpicklable=False)
    response = make_response(sensorDataJson)
    response.content_type = 'application/json'
    return response

@app.route("/weights/<string:sensor>/name", methods=['GET'])
def getSensorName(sensor):
    sensor = escape(sensor)
    if sensor not in sensorData.keys():
        response = make_response('The requested sensor is not registered.', 404)
        return response
    return sensorData.get(sensor).name

@app.route("/weights/<string:sensor>/weight", methods=['GET'])
def getWeightData(sensor):
    sensor = escape(sensor)
    if sensor not in sensorData.keys():
        response = make_response('The requested sensor is not registered.', 404)
        return response
    return str(sensorData.get(sensor).weight)

@app.route("/weights/<string:sensor>/full", methods=['GET'])
def getFullData(sensor):
    sensor = escape(sensor)
    if sensor not in sensorData.keys():
        response = make_response('The requested sensor is not registered.', 404)
        return response
    fullWeight = sensorData.get(sensor).full
    return str(fullWeight) if fullWeight is not None else 'null' 

@app.route("/weights/<string:sensor>/empty", methods=['GET'])
def getEmptyData(sensor):
    sensor = escape(sensor)
    if sensor not in sensorData.keys():
        response = make_response('The requested sensor is not registered.', 404)
        return response
    emptyWeight = sensorData.get(sensor).empty
    return str(emptyWeight) if emptyWeight is not None else 'null'
    
@app.route("/weights/<string:sensor>/level", methods=['GET'])
def getContainerFillLevel(sensor):
    sensor = escape(sensor)
    if sensor not in sensorData.keys():        
        response = make_response('The requested sensor is not registered.', 404)
        return response
    data = sensorData[sensor]
    if data.empty == None: 
        response = make_response('The empty container has to be weight first using the empty endpoint.', 424)
        return response
    if data.full == None: 
        response = make_response('The full container has to be weight first using the full endpoint.', 424)
        return response
    return str((data.weight - data.empty) / (data.full - data.empty))
    

# Add calibrate between full/empty or not?

@app.route("/weights/<string:sensor>/full", methods=['POST'])
def setFullContainerWeight(sensor):
    sensor = escape(sensor)
    if sensor not in sensorData.keys():
        raise KeyError("The requested sensor is not registered.")
    sensorData[sensor].full = sensorData[sensor].weight
    return str(sensorData[sensor].full)

@app.route("/weights/<string:sensor>/empty", methods=['POST'])
def setEmptyContainerWeight(sensor):
    sensor = escape(sensor)
    if sensor not in sensorData.keys():
        raise KeyError("The requested sensor is not registered.")
    sensorData[sensor].empty = sensorData[sensor].weight
    return str(sensorData[sensor].empty)


@app.route("/weights/tare", methods=['POST'])
def setTare():
    client.publish("cocktail/weight/", "TARE")
    return ""

# Send pump instructions

@app.route("/pumps/<string:sensor>/burst", methods=['POST'])
def setPumpBurst(sensor):
    sensor = escape(sensor)
    client.publish("cocktail/pumpen", "BURST" + sensor + "------")
    return ""

@app.route("/pumps/<string:sensor>/timed/<string:time>", methods=['POST'])
def setPump(sensor, time):
    sensor = escape(sensor)
    time = escape(time)
    client.publish("cocktail/pumpen", "TIMED" + sensor + "AS" + time + "------")
    return ""

# asynchronous call
@app.route("/pumps/fill/<string:sensor>/<string:amount>", methods=['POST'])
def setFillGlass(sensor, amount):
    sensor = escape(sensor)
    time = escape(amount)
    callback = request.headers.get('CPEE-CALLBACK')
    
    callback = re.sub('"', '', callback)
    Thread(target = fillGlass, args=(sensor, amount, callback)).start()
    client.publish("cocktail/pumpen", "TIMED" + sensor + "AS" + time + "------", qos=0)
    response = make_response('Ack.: Response later', 200)
    response.headers['CPEE-CALLBACK'] = 'true'
    return response

@app.route("/pumps/testput", methods=['PUT'])
def testput():
    print("PUT")
    return ""

def fillGlass(sensor, amount, callback):
    try:
        # sensor = "1"
        duration = "2"
        glass_start_weight = 0
        stop = False
        if "sensor_1111" in sensorData.keys():
            glass_start_weight = sensorData["sensor_1111"].weight
            print(f"Start weight: {glass_start_weight}")
            print(f"End Weight: {amount}")
        else:
            print("No glass was found")
            raise Exception('No glass was found')
        max_iterations = 10
        iterator = 0
        while (iterator < max_iterations) and not stop:
            iterator += 1
            print("Starting pump")
            client.publish("cocktail/pumpen", "TIMED" + sensor + "AS" + duration + "------")
            
            time.sleep(4)

            print("Current weight:")
            print(sensorData['sensor_1111'].weight)
            print("Already pumped:")
            print(sensorData['sensor_1111'].weight - glass_start_weight)
            if sensorData["sensor_1111"].weight - glass_start_weight >= int(amount):
                print("Finished pumping")
                stop = True
        print(callback)
        requests.put(callback)
        glass_start_weight = 0 # sensorData[sensor].weight;
        #isBusy = False
    except Exception as e: print(repr(e))
    #isBusy = False
