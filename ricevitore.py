import paho.mqtt.client as mqtt

def on_message(client, userdata, msg):
    print(f"📥 Dato ricevuto da {msg.topic}: {msg.payload.decode()} cm")

client = mqtt.Client()
client.on_message = on_message
client.connect("broker.hivemq.com", 1883, 60)
client.subscribe("arduino/distanza")

print("📡 In ascolto su topic 'arduino/distanza'...")
client.loop_forever()