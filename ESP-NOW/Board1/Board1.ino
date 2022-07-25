#include <WiFi.h>
#include <esp_now.h>
 

// 板子MAC地址
uint8_t MAC0[] = {0x84, 0xf7, 0x03, 0xdd, 0x3b, 0x98};
uint8_t MAC1[] = {0x84, 0xf7, 0x03, 0xdd, 0x3b, 0x80};
uint8_t MAC2[] = {0x84, 0xf7, 0x03, 0xdc, 0xf5, 0x36};

// 设置接收数据结构体
typedef struct Rdata 
{
  int ID;
  int battery;
  int power;
}Rdata;
Rdata myData;

// 设置发送数据结构体
typedef struct Tdata 
{
  int ID;
  int battery;
  int power;
}Tdata;
Tdata idData;

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
  Serial.printf("battery: %d \n", myData.battery);
  Serial.printf("power: %d \n", myData.power);

  Serial.println();
  delay(500);
}

// 注册数据发送回调函数
//返回信息是否发送成功
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
  delay(500);
}

/*int b = 0;
unsigned long previousMillis = 0;
int interval = 10000;

//每隔10秒加一的函数赋值battery
int Add(int i)
{
  unsigned long currentMillis = millis();
  if(i <= 100 && 
      currentMillis - previousMillis >= interval)
  {
    i++;
  }
  b = i;
  return b;
}
*/

void setup() 
{
  Serial.begin(115200);

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

  //注册接收设备
  memcpy(peerInfo.peer_addr, MAC0, 6);//广播地址
  // 检查设备是否配对成功
  if (esp_now_add_peer(&peerInfo) != ESP_OK) 
  {
    Serial.println("Failed to add peer");
    return;
  }
  
  //注册接收数据时的回调函数
  esp_now_register_recv_cb(OnDataRecv);
}

void loop() 
{
  // 设置要发送的数据
  idData.ID = 1;
  idData.power = 30;
  idData.battery =30;
  

  // 给接收板子发送数据
  esp_err_t result1 = esp_now_send(MAC0,
                      (uint8_t *) &idData, sizeof(idData));  
  // 检查数据是否发送成功
  if (result1 == ESP_OK)
  {
    Serial.println("Sent board0 with success");
  }
  else 
  {
    Serial.println("Error sending the data to board0");
  }
  
  delay(1000);
}
