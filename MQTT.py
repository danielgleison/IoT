import paho.mqtt.client as mqtt
import pandas as pd
from datetime import datetime

mqtt_topic = "MACC/IoT/Esp8266/Decrypted"
mqtt_broker_ip = "192.168.0.200"
#mqtt_broker_ip = "broker.mqtt-dashboard.com"
mqtt_broker_port = 1883

 
client = mqtt.Client()

data = []


# These functions handle what happens when the MQTT client connects

# to the broker, and what happens then the topic receives a message

def on_connect(client, userdata, flags, rc):

    # rc is the error code returned when connecting to the broker

    print ("Connected!")


    # Once the client has connected to the broker, subscribe to the topic

    client.subscribe(mqtt_topic)

   

def on_message(client, userdata, msg):

    # This function is called everytime the topic is published to.

    # If you want to check each message, and do something depending on

    # the content, the code to do this should be run in this function

   
    # print ("Topic: ", msg.topic + "\nMessage: " + str(msg.payload))
    # print ("Message: " + str(msg.payload))
    # client.publish(mqtt_topic,msg.payload,qos=0)
    
    now = datetime.now().strftime('%d/%m/%Y %H:%M:%S')
    temp = msg.payload.decode('utf-8')
    
    data.append([now, temp])
    df = pd.DataFrame(data, columns = ['Time','Decrypted'])
    df.to_csv('dataset_IoT.csv', mode = 'a', header = False)
    print(df)
  

    # The message itself is stored in the msg variable

    # and details about who sent it are stored in userdata
 

# Here, we are telling the client which functions are to be run

# on connecting, and on receiving a message

client.on_connect = on_connect

client.on_message = on_message

 

# Once everything has been set up, we can (finally) connect to the broker

# 1883 is the listener port that the MQTT broker is using

client.connect(mqtt_broker_ip, mqtt_broker_port)
 

# Once we have told the client to connect, let the client object run itself

client.loop_forever()

client.disconnect()