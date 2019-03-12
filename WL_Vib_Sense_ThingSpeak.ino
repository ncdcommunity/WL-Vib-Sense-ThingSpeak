#include <WiFi.h>
#include <PubSubClient.h>
#include "FS.h"
#include "SPIFFS.h"
#include <EEPROM.h>
#include <WebServer.h>
#include <HardwareSerial.h>
#define RXD2 16
#define TXD2 17
#define MESSAGE_MAX_LEN 256



//const char *messageData = "\"field1=\"\"%d\"\"field2=\"\"%d\"\"field3=\"\"%.1f\"";

//--------- ThingSpeak MQTT parameters------------//
const char* mqtt_server = "mqtt.thingspeak.com";
char mqttUserName[] = "MQTTvbshightime";
char mqttPass[] = "VWFUWQUOI9WJXS0L";

//--------- ThingSpeak Channel parameters------------//
const char writeAPIKey[] = "8A5PMTM3BWUXTOOO";
static long channelID = 726850;
const char readAPIKey[] = "YUFI9SY471YAR7UC";

//*************WiFi Credentials*****************//
const char* WiFiSSID = "Dcube_web";
const char* WiFiPass = "D@123456";

WiFiClient wifiClient;
 

//--------- Temp parameters------------//
int16_t Xrms; 
int16_t Yrms; 
float tempC;

//*********ZigBee Frame**************/
uint8_t data[54];
int k = 10;
int i;

int tick=0;
WebServer server(80);

//class object for publishing and subscribing to mqtt broker
PubSubClient mqttCli;


//---------Setup function------------//
void setup() {
  // put your setup code here, to run once:

Serial.begin(9600);
Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2); // pins 16 rx2, 17 tx2, 19200 bps, 8 bits no parity 1 stop bit
  
while(!Serial);
WiFi.persistent(false);
WiFi.disconnect(true);
    

 mqttCli.setServer(mqtt_server,1883);
 mqttCli.setClient(wifiClient);
// mqttCli.setCallback(callback);
reconnectWiFi();   
 reconnectMQTT();
}

//---------loop function------------//
void loop() {
  if (Serial2.available())
  {
    data[0] = Serial2.read();
    delay(k);
   if(data[0]==0x7E)
    {
    Serial.println("Got Packet");
    while (!Serial2.available());
    for ( i = 1; i< 55; i++)
      {
      data[i] = Serial2.read();
      delay(1);
      }
    if(data[15]==0x7F)  /////// to check if the recive data is correct
      {
  if(data[22]==0x08)  //////// make sure the sensor type is correct
  {
  int16_t rms_x = ((uint16_t)(((data[24])<<16) + ((data[25])<<8) + (data[26]))/100);
  int16_t rms_y = ((uint16_t)(((data[27])<<16) + ((data[28])<<8) + (data[29]))/100);
  int16_t rms_z = ((uint16_t)(((data[30])<<16) + ((data[31])<<8) + (data[32]))/100);
  int16_t max_x = ((uint16_t)(((data[33])<<16) + ((data[34])<<8) + (data[35]))/100);
  int16_t max_y = ((uint16_t)(((data[36])<<16) + ((data[37])<<8) + (data[38]))/100);
  int16_t max_z = ((uint16_t)(((data[39])<<16) + ((data[40])<<8) + (data[41]))/100);

  int16_t min_x = ((uint16_t)(((data[42])<<16) + ((data[43])<<8) + (data[44]))/100);
  int16_t min_y = ((uint16_t)(((data[45])<<16) + ((data[46])<<8) + (data[47]))/100);
  int16_t min_z = ((uint16_t)(((data[48])<<16) + ((data[49])<<8) + (data[50]))/100);

  
  float cTemp = ((((data[51]) * 256) + data[52]));
  float battery = ((data[18] * 256) + data[19]);
  float voltage = 0.00322 * battery;
  Serial.print("Sensor Number  ");
  Serial.println(data[16]);
  Serial.print("Sensor Type  ");
  Serial.println(data[22]);
  Serial.print("Firmware Version  ");
  Serial.println(data[17]);
  Serial.print("Temperature in Celsius :");
  Serial.print(cTemp);
  Serial.println(" C");
  
  Serial.print("RMS vibration in X-axis :");
  Serial.print(rms_x);
  Serial.println(" g");
  Serial.print("RMS vibration in Y-axis :");
  Serial.print(rms_y);
  Serial.println(" g");
  Serial.print("RMS vibration in Z-axis :");
  Serial.print(rms_z);
  Serial.println(" g");

  Serial.print("Min vibration in X-axis :");
  Serial.print(min_x);
  Serial.println(" g");
  Serial.print("Min vibration in Y-axis :");
  Serial.print(min_y);
  Serial.println(" g");
  Serial.print("Min vibration in Z-axis :");
  Serial.print(min_z);
  Serial.println(" g"); 

  Serial.print("ADC value:");
  Serial.println(battery);
  Serial.print("Battery Voltage:");
  Serial.print(voltage);
  Serial.println("\n");
  if (voltage < 1)
          {
    Serial.println("Time to Replace The Battery");
          }
        }
      }
else
{
      for ( i = 0; i< 54; i++)
    {
      Serial.print(data[i]);
      Serial.print(" , ");
      delay(1);
    }
}
}
  } 
    if(tick >= 30)   // publish to topic every 5seconds
    {
        tick=0;
  Serial.println("taskMQTTStarted");
      if(!mqttCli.connected()){
          Serial.println("Client not connected");
          reconnectMQTT();
        } 
       
       String topicString ="channels/"+String(channelID)+"/publish/"+String(writeAPIKey);
       int topicLength = topicString.length()+1;
       char topicBuffer[topicLength];
       topicString.toCharArray(topicBuffer,topicLength+1);
       Serial.println(topicBuffer);

    //   char messagePayload[MESSAGE_MAX_LEN];
//       snprintf(messagePayload,MESSAGE_MAX_LEN, messageData,Xrms,Yrms,tempC);
  //     Serial.println(messagePayload);
       String dataString = String("field1="+ Xrms.c_str() + "&field2=" + Yrms.c_str() + "&field3=" + tempC.c_str());
       int dataLength = dataString.length()+1;
       byte dataBuffer[dataLength];
       dataString.getBytes(dataBuffer,dataLength);
       mqttCli.beginPublish(topicBuffer,MESSAGE_MAX_LEN,false);

       Serial.println(mqttCli.write(dataBuffer,dataLength) ? "published" : "published failed");
      // Serial.println(mqttCli.write(messagePayload,MESSAGE_MAX_LEN) ? "published" : "published failed");

       mqttCli.endPublish();
       
       mqttCli.loop();  
  }
    vTaskDelay(1000 / portTICK_RATE_MS); 
    tick++; 
}

    

void reconnectMQTT(){
   Serial.println("setting up mqtt");
   while(!mqttCli.connected()){
     if(mqttCli.connect("ESP8266Client123456789")==true){
        Serial.println("connected");
        String subTopic = String("channels/"+ String(channelID) + "/subscribe/json/" + String(readAPIKey));
        int subTopicLength = subTopic.length()+1;
        char subTopicBuffer[subTopicLength];
        subTopic.toCharArray(subTopicBuffer,subTopicLength);
        
        String pubMessage = "status=MQTTPUBLISH";
        String pubTopic =String("channels/"+String(channelID)+"/publish/"+String(writeAPIKey));
        int pubTopicLength = pubTopic.length()+1;
        char pubTopicBuffer[pubTopicLength];
        pubTopic.toCharArray(pubTopicBuffer,pubTopicLength);
        //Publish to MQTT Broker 
        Serial.println(mqttCli.publish(pubTopicBuffer, pubMessage.c_str()) ? "Published" : "NotPublished");
        //Subscribe to MQTT Broker
        
        Serial.println(mqttCli.subscribe(subTopicBuffer) ? "Subscribed" : "Unsbscribed"); 
       }else{
           Serial.print("failed, rc=");
           Serial.println(mqttCli.state());
           delay(1000);
        }
    }
  }

  void reconnectWiFi(){
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();        
  delay(400);
  WiFi.begin(WiFiSSID,WiFiPass);
  int counter = 0;
  while (WiFi.status() != WL_CONNECTED)
  {   
      delay(500);
      Serial.print(".");
      if(counter == 100){
          ESP.restart();
        }
        counter++;  
  }
  }
