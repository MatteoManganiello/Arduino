import paho.mqtt.client as mqtt
import time
import random

broker = "broker.hivemq.com"
port = 8000
topic = "arduino/distanza"

client = mqtt.Client()
client.connect(broker, port, 60)

alarm_count = 0

while True:
    distanza = random.randint(5, 100)

    print(f"[SIMULAZIONE] Distanza rilevata: {distanza} cm")

    if distanza < 20:
        alarm_count += 1
        print(" Buzzer ATTIVO (distanza critica)")

        if alarm_count == 9:
            print(" Buzzer NEGATIVO ATTIVO per 10 secondi!")
            for i in range(10):
                print(f" Secondi trascorsi: {i + 1}")
                time.sleep(1)
            print(" Buzzer NEGATIVO SPENTO")
            alarm_count = 0
    else:
        print(" Distanza sicura - buzzer spento")

    client.publish(topic, str(distanza))
    print(f" Pubblicato su MQTT: {distanza} cm\n")

    time.sleep(2)
