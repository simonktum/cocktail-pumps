Below is a copy of the original readme by Quirin Sailer

----

# Relay Ansteuerung
Das Hauptziel dieses Projekts ist die Erstellung eines Systems, das das unabhängige Ansteuern von bis zu acht Relays über das MQTT-Protokoll ermöglicht.

Jedes dieser Relays kann dann genutzt werden, um verschiedene Lasten, wie beispielsweise Pumpen, zu steuern. Die Steuerung der Relays erfolgt durch den ESP32 Mikrocontroller, der MQTT-Nachrichten von einem MQTT-Broker empfängt.

Jede Nachricht gibt an, welches Relay (identifiziert durch eine Nummer zwischen 0 und 7) aktiviert oder deaktiviert werden soll. Die Nachrichten werden über das MQTT-Topic "/cmnd/Relay/Status" gesendet.

## Kickstart Bedienungsanleitung
Eine kurze Anleitung:
1. Prüfen, dass alle Anbauten, wie Pumpen korrekt verkabelt sind
2. Prüfen dass die Konfiguration des ESPs korret ist ([siehe Anpassungen](#anpassungen))
3. Spannungsversorgung herstellen ([siehe Energieversorgnung](#energieversorgung))
4. MQTT Nachrichten an ESP senden ([siehe Konzept](#konzept))


## Anpassungen
In dem gegebenen Code können verschiedene Anpassungen vorgenommen werden, um ihn auf spezifische Anforderungen zuzuschneiden. Hier sind einige der Bereiche, in denen Anpassungen möglich sind:

- **WiFi-Konfiguration:** Die Konstanten für die SSID und das Passwort des WiFi-Netzwerks, mit dem sich der ESP32 verbinden soll, können am Anfang des Codes angepasst werden. Diese Konstanten definieren den Namen und das Passwort des Netzwerks, das der ESP32 zum Verbinden verwenden soll.

- **MQTT-Konfiguration:** Ebenfalls am Anfang des Codes können die Konstanten für den MQTT-Server, den Port und das Topic angepasst werden. Diese Konstanten definieren, wo sich der MQTT-Broker befindet (der Server und der Port) und welches Topic der ESP32 abonnieren und auf das er hören soll.

- **Pin-Konfiguration:** Die genutzten Pins des ESP32 können angepasst werden. Es ist wichtig zu beachten, dass nur die digitalen GPIO-Pins des ESP32 verwendet werden können. Diese Pins haben eine D-Nummer, die im Pinout des ESP32 angegeben ist.

Es ist zu beachten, dass die D-Nummern im Code anders nummeriert sind als im Pinout. Zum Beispiel ist D1 im Code Pin Nummer 5 und D2 ist Pin Nummer 4. Deshalb ist es wichtig, das Pinout des ESP32 zu konsultieren, wenn Sie die Pin-Konfiguration im Code ändern. (https://i0.wp.com/randomnerdtutorials.com/wp-content/uploads/2018/08/ESP32-DOIT-DEVKIT-V1-Board-Pinout-36-GPIOs-updated.jpg?w=750&quality=100&strip=all&ssl=1)

Das Flashen des ESPs erfolgt mittels der Arduino IDE. Tutorials hierfür finden sich im Internet z.b. https://randomnerdtutorials.com/getting-started-with-esp32/


## Konzept

In diesem Szenario agiert ein ESP32 als ein MQTT-Client, der sich mit einem spezifizierten WLAN verbindet und auf ein bestimmtes MQTT-Topic, nämlich "/cmnd/Relay/Status", lauscht. Der ESP32 wird über dieses Topic gesteuert, was bedeutet, dass Befehle oder Aktionen, die durch den ESP32 ausgeführt werden sollen, an dieses Topic gesendet werden.

Die Aktionen, die der ESP32 ausführt, werden durch den Payload des MQTT-Nachrichten bestimmt. Wenn der Payload das Wort `ON` enthält, gefolgt von einer Zahl zwischen 0 und 7, aktiviert der ESP32 das entsprechende Relay. Diese Relays sind durchnummeriert und entsprechen den im Payload angegebenen Zahlen. Analog dazu, wenn der Payload das Wort `OFF` enthält, deaktiviert der ESP32 das entsprechende Relay. Beispielsweise kann mit der Message `ON14` das Relay 1 und 4 angeschaltet werden, während `OFF034` Die Relays 0,3 und 4 ausschaltet

Zusätzlich verfügt der ESP32 über einen seriellen Ausgang, der zur Protokollierung von Informationen verwendet wird. Diese Protokollierung erfolgt bei einer Baudrate von 9600.

## Aufbau

### Aufbau der Hardware
#### Verwendete Komponenten
Die Hauptkomponenten dieses Projekts sind:

 - Ein ESP32 Mikrocontroller (5)
 - Ein 8-Kanal-Relays-Modul (4)
 - Bis zu 8 Pumpen
 - Ein Gehäuse, das den ESP32 und das Relays-Modul beherbergt
 - Ein USB-C Input (2)
 - Ein Micro USB Input (1)
 - Ein Breadboard
 - Eine USB-C PD Platine (3)
 - Verschiedene Kabel und Stecker

#### ESP32 und Gehäuse

Der ESP32 ist das Herzstück dieses Projekts. Er ist zusammen mit allen anderen Komponenten (außer der Pumpen) in dem Gehäuse eingebaut.

#### Energieversorgung
Das Gehäuse hat 2 Eingänge, ein Micro USB und ein USB-C Anschluss.
Über den Micro USB Anschluss (1) wird die Betriebsspannung für den ESP32 bereitgestellt. Außerdem kann über diesen Anschluss der ESP32 programmiert werden. Desweiteren werden die Relays mit den 5V angesteuert.

Über den USB-C Anschluss (2) wird per Power Delivery (PD) die Betriebsspannung für die Pumpen bzw. Anbaugeräte bereitgestellt.
Die Pumpen sind für eine Einsatzspannung von 12V ausgelegt, längere Tests haben aber ergeben, dass ein Einsatz bei 20V allerdings ebenfalls bedenkenlos durchgeführt werden können.
Um den Aufbau des Systems zu vereinfachen haben wir uns dazu entschieden die Stromversorgung mittels [Power Delivery](https://de.wikipedia.org/wiki/Universal_Serial_Bus#Energieversorgung) zu realisieren, da so der Bedarf an einen externen Labornetzteil wegfällt.

Leider ist die verwendete Platine, welche USB-C Power Delivery bereistellt nicht optimal geeignet. So startet die Platine mit einer Leistung von nur 5V. Um die Spannung zu erhöhen, muss der Deckel des Gehäuses geöffnet werden und der Button auf der Platine 3 mal gedrückt werden. Eine kleine LED auf der Platine zeigt die Spannung an:
- Rot 5V
- Grün 9V
- Grün? 15V
- Hellblau 20V

Für die Pumpen kann sowohl 15V, als auch 20V verwendet werden. Power Delivery ist für maximal 100W ausgelegt, was mehr als ausreichend ist.

Theoretisch wäre es möglich den ESP32 auch mittels den Power Delivery des USB-C Anschlusses zu befeuern, allerdings wäre dann ein Step Down Converter nötig, welcher die Spannung von 20V auf 5V reduziert, desweiteren haben Motoren relativ hohe Anlaufströme, welche die Funktionsfähigkeit des ESPs beeinträchtigen könnten. Das Programmieren des ESPs könnte man per Over-the-Air Update (OTA) durchführen und hätte damit auch keinen Bedarf an einen physischen Micro USB. Allerdings kann das OTA Update erfahrungsgemäß auch fehlschlagen, was ein Ausbau des ESPs erfordern würde. Aus diesen Gründen haben wir uns dafür entschieden den ESP über ein eigenen Anschluss anzusteuern.

![LED bei 5V](./5V.jpg)
*LED bei 5V*
![LED bei 9V](./9V.jpg)
*LED bei 9V*
![LED bei 15V](./15V.jpg)
*LED bei 15V*
![LED bei 20V](./20V.jpg)
*LED bei 20V*


#### Breadboard und Relays-Modul

Im Inneren des Gehäuses ist der ESP32 auf einem Breadboard aufgesteckt, das zwei Spannungsleitungen hat:

- Die obere Leitung führt 5V für das Relays-Modul (6)
- Die untere Leitung führt 20V für die Pumpen (7)

Das 8-Kanal-Relays-Modul ist so verdrahtet, dass es Masse schaltet. Dies ermöglicht den Einsatz von Spannungen bis zu 50V. Falls zusätzliche Spannung benötigt wird, kann diese einfach hinzugefügt werden, indem die Masse der zusätzlichen Spannung mit der 20V-Masse verbunden wird. Das Masse-Schalten verhindert jedoch die Verwendung eines Schutzleiters, daher ist die maximale Spannung auf 50V begrenzt [Rechtlicher Hinweis](https://www.bgetem.de/arbeitssicherheit-gesundheitsschutz/themen-von-a-z-1/elektrische-gefaehrdungen-1/elektrotechnische-arbeiten-qualifikation/arbeiten-an-elektrischen-anlangen/arbeiten-unter-spannung).

#### GPIO und Pumpen

Der ESP32 steuert das 8-Kanal-Relays-Modul über seine GPIOs an. Der 20V Input ist auf mehrere Stecker aufgeteilt, um eine Überlastung einer einzelnen Leitung zu verhindern und mögliche Wärmeentwicklung entgegenzuwirken.

Der genaue Aufbau ist im Folgenden dargestellt:

![Schematischer Aufbau](./BreadBoard.png)

![Blick ins Ghäuse](./Gehaeuse1_edited.jpg)





Bitte beachten, dass die korrekte Verschaltung der Komponenten für das ordnungsgemäße Funktionieren des Systems unerlässlich ist. Es ist wichtig, das korrekte Pinout und die spezifischen Anforderungen der verwendeten Hardwarekomponenten zu beachten.



### Ausgänge
Das Gehäuse besitzt insgesamt 16 Ausgänge, wobei immer 2 ein paar ergeben. Am Gehäuse findet sich ander Vorderseite die Nummerierung der einzelnen Ausgänge und deren Poralität. Zu beachten ist, dass nur die Masse markiert ist.

![Gehäuse von außen](./Gehause_aussen.jpg)


### Aufbau des Codes
**1. Konstanten und Variable:**
Der nächste Abschnitt definiert einige Konstanten für den WiFi-Zugriff und den MQTT-Server sowie die Pins, an die die Geräte angeschlossen sind. Es wird ein AsyncMqttClient-Objekt namens mqttClient erstellt.

**2. WiFi und MQTT Verbindung:**
Es werden Funktionen definiert, um eine Verbindung zum WiFi (connectToWifi()) und zum MQTT Server (connectToMqtt()) herzustellen.

**3. WiFi Ereignisse:**
Die Funktion WiFiEvent behandelt zwei spezifische WiFi-Ereignisse: Wenn das Gerät eine IP-Adresse vom Router erhält (was bedeutet, dass es erfolgreich verbunden ist) und wenn das Gerät die Verbindung zum WiFi verliert.

**4. MQTT Ereignisse:**
In den Funktionen onMqttConnect und onMqttMessage wird definiert, was geschehen soll, wenn das Gerät eine Verbindung zum MQTT-Server herstellt bzw. wenn eine MQTT-Nachricht empfangen wird. In onMqttMessage wird auf bestimmte Inhalte in der empfangenen Nachricht geprüft, um die entsprechenden Aktionen auszuführen (in diesem Fall das Einschalten oder Ausschalten von Pins).

**5. Setup Funktion:**
In der Setup-Funktion werden die serielle Kommunikation initialisiert, die Pin-Modi festgelegt und Ereignishandler für WiFi- und MQTT-Ereignisse registriert. Zudem wird versucht, eine Verbindung zum WiFi herzustellen.

**6. Hauptschleife (Loop Funktion):**
Die Hauptschleife (loop()) ist in diesem Skript leer, da alle Aktionen durch Ereignishandler ausgelöst werden, die auf eingehende WiFi- oder MQTT-Ereignisse reagieren.

Bei Fragen gerne eine Email an:\
[pumpenmann@sailer.software](pumpenmann@sailer.software)