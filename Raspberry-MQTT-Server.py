import paho.mqtt.client as mqtt
import pandas as pd
from datetime import datetime

mqtt_topic = "MACC/IoT/Esp8266/Decrypted"
mqtt_broker_ip = "raspberrypi"
mqtt_broker_port = 1883

 
client = mqtt.Client()

data = []

def on_connect(client, userdata, flags, rc):

    print ("Connected!")

    client.subscribe(mqtt_topic)

   

def on_message(client, userdata, msg):

  
    now = datetime.now().strftime('%d/%m/%Y %H:%M:%S')
    temp = msg.payload.decode('utf-8')
    
    data.append([now, temp])
    df = pd.DataFrame(data, columns = ['Time','Decrypted'])
    df.to_csv('dataset_IoT.csv', mode = 'a', header = False)
    print(df)
  
client.on_connect = on_connect

client.on_message = on_message

client.connect(mqtt_broker_ip, mqtt_broker_port)
 
client.loop_forever()

client.disconnect()
