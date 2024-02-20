#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "Jay";
const char* password = "9930046480";

float X_acc = 0;
float Y_acc = 0;
float Z_acc = 0;
float X_gyro = 0;
float Y_gyro = 0;
float Z_gyro = 0;
unsigned char activity[7];

// Your Domain name with URL path or IP address with path
const char* serverName = "http://18.234.117.74:8080/sendData";

unsigned long lastTime = 0;
unsigned long timerDelay = 1000;
String response;

void setup() {
  Serial.begin(115200);
  Wire.begin(8);
  Wire.onReceive(receiveEvent);

  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Timer set to 5 seconds (timerDelay variable), it will take 5 seconds before publishing the first reading.");
}

void loop() {
  // Your main code logic can be placed here if needed
}

void receiveEvent(int howMany) {
  if (howMany >= sizeof(float) * 6 + 1) {
    float receivedData[6];

    Wire.readBytes(reinterpret_cast<uint8_t*>(receivedData), sizeof(float) * 6);

    float x_acc = receivedData[0];
    float y_acc = receivedData[1];
    float z_acc = receivedData[2];
    float x_gyro = receivedData[3];
    float y_gyro = receivedData[4];
    float z_gyro = receivedData[5];

    uint8_t activityIndex = Wire.read();

    Serial.println("here");
    handleReceivedData(x_acc, y_acc, z_acc, x_gyro, y_gyro, z_gyro, activityIndex);
    sendDataToServer();
  }
}

void handleReceivedData(float x_acc, float y_acc, float z_acc, float x_gyro, float y_gyro, float z_gyro, uint8_t activityIndex) {
  switch (activityIndex) {
    case 0:
      strncpy((char*)activity, "Run", sizeof(activity));
      break;
    case 1:
      strncpy((char*)activity, "Stand", sizeof(activity));
      break;
    case 2:
      strncpy((char*)activity, "Walk", sizeof(activity));
      break;
    default:
      strncpy((char*)activity, "Unknown", sizeof(activity));
      break;
  }

  X_acc = x_acc;
  Y_acc = y_acc;
  Z_acc = z_acc;
  X_gyro = x_gyro;
  Y_gyro = y_gyro;
  Z_gyro = z_gyro;

  // Print the received values
  Serial.print("Received IMU Readings: ");
  Serial.print("X_acc: ");
  Serial.print(x_acc);
  Serial.print(", Y_acc: ");
  Serial.print(y_acc);
  Serial.print(", Z_acc: ");
  Serial.println(z_acc);
  Serial.print("Received IMU Readings: ");
  Serial.print("X_gyro: ");
  Serial.print(x_gyro);
  Serial.print(", Y_gyro: ");
  Serial.print(y_gyro);
  Serial.print(", Z_gyro: ");
  Serial.println(z_gyro);

  // Print the detected posture
  Serial.print("Detected Posture: ");
  Serial.println((char*)activity);


  // // Print the detected posture
  // Serial.print("Detected Posture: ");
  // switch (postureIndex) {
  //   case 0:
  //     Serial.println("Unknown Posture");
  //     break;
  //   case 1:
  //     Serial.println("Supine Posture");
  //     break;
  //   case 2:
  //     Serial.println("Prone Posture");
  //     break;
  //   case 3:
  //     Serial.println("Side Posture");
  //     break;
  //   case 4:
  //     Serial.println("Sitting Posture");
  //     break;
  //   default:
  //     Serial.println("Unknown Posture");
  //     break;
  // }
}

void sendDataToServer() {
  if ((millis() - lastTime) > timerDelay) {
    if (WiFi.status() == WL_CONNECTED) {
      //String url = String(serverName) + "?x=" + xaxis + "&y=" + yaxis + "&z=" + zaxis + "&posture=" + String((char*)activity);
      String url = String(serverName) + "?x_acc=" + X_acc +
             "&y_acc=" + Y_acc + "&z_acc=" + Z_acc +
             "&x_gyro=" + X_gyro + "&y_gyro=" + Y_gyro +
             "&z_gyro=" + Z_gyro + "&Activity=" + String((char*)activity);
      //Serial.println(url);
      response = httpGETRequest(url.c_str());
      Serial.println(response);
    } else {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();
  }
}

String httpGETRequest(const char* serverName) {
  HTTPClient http;
  http.begin(serverName);
  int httpResponseCode = http.GET();
  String payload = "{}";
  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  http.end();
  return payload;
}
