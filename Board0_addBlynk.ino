#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID "TMPLvj6LUM_b"
#define BLYNK_DEVICE_NAME "Quickstart Template"
#define BLYNK_AUTH_TOKEN "K0btl6wQPwnO9Rw55cmyW6HZou3BTEUf"
#define BLYNK_WIFI_SSID "1004B"
#define BLYNK_WIFI_PASS "WOSHIWENPI"

#include <PubSubClient.h>
#include <BlynkSimpleEsp32.h>
//#include <SimpleTimer.h>
//#include <Blynk.h>
#include <WiFi.h>
#include <esp_now.h>

// 板子MAC地址
uint8_t MAC0[] = {0x84, 0xf7, 0x03, 0xdd, 0x3b, 0x98};
uint8_t MAC1[] = {0x84, 0xf7, 0x03, 0xdd, 0x3b, 0x80};
uint8_t MAC2[] = {0x84, 0xf7, 0x03, 0xdc, 0xf5, 0x36};

// MQTT Broker
const char *mqtt_broker = "broker.emqx.io";
const char *topic = "esp32pi";
const char *mqtt_username = "emqx";
const char *mqtt_password = "123456";
const int mqtt_port = 1883;

//注册client客户端
WiFiClient espClient;
PubSubClient client(espClient);

//全局变量向mqtt服务器发送string类型的数据
char buff1[12];
String buff2;

//注册计时的结构体
//SimpleTimer timer;

/*---------------------------------------------------------------------*/

// 设置接收数据结构体
typedef struct Rdata 
{
  int ID;
  int battery;
  int power;
}Rdata;
Rdata myData;

//创建一个结构来储存每个板的读数
Rdata board1;
Rdata board2;
Rdata board3;//后期可用循环写入

//创建一个数组储存不同的板子,方便写入读取
Rdata Board[2] = {board1, board2};

// 设置发送数据结构体
typedef struct Tdata 
{
  int ID;
  int battery;
  int power;
}Tdata;
Tdata idData;

//每个时间段发送
void myTimer()
{
  Blynk.virtualWrite(myData.ID, myData.battery);
}

// 注册数据接收回调函数
void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) 
{
  char macStr[18];
  Serial.print("Packet from: ");
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.println(macStr);//输出MAC地址
  
  memcpy(&myData, incomingData, sizeof(myData));
  Serial.printf("board_ID: %u; %u 字节\n", myData.ID, len);
  Board[myData.ID -1].battery = myData.battery;//接收到的值送到对应的结构体变量中
  Board[myData.ID -1].power = myData.power;

  //接收的数据整理为string类型向MQTT发送
  sprintf(buff1,"%d %d %d.",myData.ID, myData.power, myData.battery);
  buff2 = buff1;

  //上传数据至Blynk
  // 配置向blynk发送信息的时间间隔
  //timer.setInterval(1000, myTimer);// 每一秒向blynk发送数据
  delay(1000);
  int i = myData.ID;
  Blynk.virtualWrite(i, myData.battery);

  //串口打印
  //Serial.printf("battery: %d \n", Board[myData.ID -1].battery);
  //Serial.printf("power: %d \n", Board[myData.ID -1].power);
  Serial.printf("battery: ");
  Serial.println(Board[myData.ID -1].battery);
  Serial.printf("power: ");
  Serial.println(Board[myData.ID -1].power);
  Serial.println();
  //delay(500);
}


// 注册数据发送回调函数
// 返回信息是否发送成功
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  char macStr[18];
  Serial.print("Packet to: ");
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.println(macStr);
  Serial.print("Send status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  Serial.println();
  //delay(500);
}

//检查数据是否发送成功
void CheckSend(int state, int i)
{
  if (state == ESP_OK)
  {
    Serial.printf("Sent board %d with success\n", i);
  }
  else 
  {
    Serial.printf("Error sending the data to board %d\n", i);
  }
  delay(100); 
}

//返回服务器接收状态
void callback(char *topic, byte *payload, unsigned int length)
{
 Serial.print("Message arrived in topic: ");
 Serial.println(topic);
 Serial.print("Message:");
 for (int i = 0; i < length; i++) 
 {
     Serial.print((char) payload[i]);
 }
 Serial.println();
 Serial.println("-----------------------");
}


/*-----------------------------------------------------------------------*/

void setup() 
{
  Serial.begin(115200);
  
  //配置Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, BLYNK_WIFI_SSID, BLYNK_WIFI_PASS);
  
  // 初始化 ESP-NOW
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != 0) 
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // 注册发送数据回调函数
  // MAC 层成功接收到数据，则该函数将返回 ESP_NOW_SEND_SUCCESS 事件
  esp_now_register_send_cb(OnDataSent);
  
   // 绑定数据接收端 注册通信频道
  esp_now_peer_info_t peerInfo;
  peerInfo.channel = 0;//通道
  peerInfo.encrypt = false;//是否加密

  //注册第一个设备
  memcpy(peerInfo.peer_addr, MAC1, 6);//广播地址
  // 检查设备是否配对成功
  if (esp_now_add_peer(&peerInfo) != ESP_OK) 
  {
    Serial.println("Failed to add peer1");
    return;
  }
   //注册第二个设备
  memcpy(peerInfo.peer_addr, MAC2, 6);//广播地址
  // 检查设备是否配对成功
  if (esp_now_add_peer(&peerInfo) != ESP_OK) 
  {
    Serial.println("Failed to add peer2");
    return;
  }
  
  //注册接收数据时的回调函数
  esp_now_register_recv_cb(OnDataRecv);

  //连接mqtt服务器
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);
  
  //判断是否连接成功
  while (!client.connected()) 
  {
     String client_id = "esp32-client-";
     client_id += String(WiFi.macAddress());
     Serial.printf("The client %s connects to the public mqtt broker\n", client_id.c_str());
     if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) 
     {
         Serial.println("Public emqx mqtt broker connected");
     } else 
     {
         Serial.print("failed with state ");
         Serial.print(client.state());
         delay(2000);
     }
  }
  
  // publish and subscribe
  client.subscribe(topic);
  
}

void loop() 
{
  //启动BLYNK
  Blynk.run();

  //runs BlynkTimer
  //timer.run();

  //
  client.loop();
  client.publish(topic, "Publish new data");
  client.publish(topic, buff1);

  // 设置要发送的数据
  idData.ID = 0;
  idData.power = 100; //·random(0, 50);
  idData.battery = 100; // random(0, 50);
  
  // 给第一块板子发送数据
  esp_err_t result1 = esp_now_send(MAC1,
                      (uint8_t *) &idData, sizeof(idData));  
  // 检查数据是否发送成功
  CheckSend(result1, 1);
  
   // 给第二块板子发送数据
  esp_err_t result2 = esp_now_send(MAC2,
                      (uint8_t *) &idData, sizeof(idData));  
  // 检查数据是否发送成功
  CheckSend(result2, 2);

}
