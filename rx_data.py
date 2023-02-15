import psycopg2
import time
import json
from paho.mqtt import client as mqtt
import random
from datetime import datetime
from pytz import timezone

import Credentials as c

tz = timezone('EST')

mydb = psycopg2.connect(
        user = c.POSTGRES_USER,
        password = c.POSTGRES_PASSWORD,
        host = c.POSTGRES_HOST,
        port = c.POSTGRES_PORT,
        database = c.POSTGRES_DATABASE
        )

cursor = mydb.cursor()

broker = c.MQTT_BROKER
port = c.MQTT_PORT
sub_topic = c.MQTT_SUB_TOPIC
pub_topic = c. MQTT_PUB_TOPIC
client_id = f'python-mqtt-{random.randint(0, 100)}'
username = c.MQTT_USERNAME
password = c.MQTT_PASSWORD

def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Connected with result code " + str(rc))
        client.subscribe(sub_topic, 0)
    else:
        print("Failed to connect to MQTT broker, return code ", rc)
        print("\n")

def on_message(client, userdata, msg):
    message = msg.payload.decode()
    #print(message)
    json_message = json.loads(message)
    #print(json_message)
    number_value = [json_message[v] for v in json_message]
    #print(number_value)
    try:
        postgres_insert_query = """ INSERT INTO sensor_telemetry(
            water_temp_c,
            humidity,
            temp_c,
            temp_f,
            pH,
            -- ir,
            -- full_spectrum,
            lux
            )
            VALUES({}, {}, {}, {}, {}, {});""".format(
                        number_value[0],
                        number_value[1],
                        number_value[2],
                        number_value[3],
                        number_value[4],
                        number_value[5]
                        #number_value[6],
                        #number_value[7]
                        )
        query = cursor.mogrify(postgres_insert_query)
        cursor.execute(query)
        mydb.commit()
        print("query inserted", datetime.now(tz))

    except Exception as e:
        print("No inserty: ", e)

client = mqtt.Client()
client.connect(broker, port)
client.on_connect = on_connect
client.on_message = on_message
client.loop_start()
time.sleep(5)
client.disconnect()