#include <TensorFlowLite.h>
#include <Wire.h>
#include <Arduino_LSM9DS1.h>
#include "main_functions.h"
#include "model.h"
#include "output_handler.h"
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_log.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"

namespace {
  const tflite::Model* model = nullptr;
  tflite::MicroInterpreter* interpreter = nullptr;
  TfLiteTensor* input = nullptr;
  TfLiteTensor* output = nullptr;
  int inference_count = 0;

  constexpr int kTensorArenaSize = 2000;
  alignas(16) uint8_t tensor_arena[kTensorArenaSize];
}

// Define the default sensor type (1 for gyroscope and 2 for accelerometer)
int sensor_type = 1;

char *classes[3] = {"RUN", "STAND", "WALK"};

void initializeModel() {
  Serial.begin(115200);
  Wire.begin();
  while (!Serial);
  Serial.println("Started");

  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }

  tflite::InitializeTarget();

  model = tflite::GetModel(g_model);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    MicroPrintf("Model provided is schema version %d not equal to supported version %d.",
                model->version(), TFLITE_SCHEMA_VERSION);
    return;
  }

  static tflite::AllOpsResolver resolver;
  static tflite::MicroInterpreter static_interpreter(
      model, resolver, tensor_arena, kTensorArenaSize);
  interpreter = &static_interpreter;

  TfLiteStatus allocate_status = interpreter->AllocateTensors();
  if (allocate_status != kTfLiteOk) {
    MicroPrintf("AllocateTensors() failed");
    return;
  }

  input = interpreter->input(0);
  output = interpreter->output(0);
  inference_count = 0;
}

void setup() {
  initializeModel();
}

void loop() {
  float x_acc, y_acc, z_acc;
  float x_gyro, y_gyro, z_gyro;

  // Read accelerometer data
  if (IMU.accelerationAvailable()) {
    IMU.readAcceleration(x_acc, y_acc, z_acc);
  }

  // Read gyroscope data
  if (IMU.gyroscopeAvailable()) {
    IMU.readGyroscope(x_gyro, y_gyro, z_gyro);
  }

  // Quantize the input from floating-point to integer
  input->data.f[0] = x_acc;
  input->data.f[1] = y_acc;
  input->data.f[2] = z_acc;
  input->data.f[3] = x_gyro;
  input->data.f[4] = y_gyro;
  input->data.f[5] = z_gyro;

  Serial.println(x_acc);
  Serial.println(y_acc);
  Serial.println(z_acc);
  Serial.println(x_gyro);
  Serial.println(y_gyro);
  Serial.println(z_gyro);

  TfLiteStatus invoke_status = interpreter->Invoke();
  if (invoke_status != kTfLiteOk) {
    Serial.println("Invoke failed");
    return;
  }

  // Determine the detected posture class based on the inference results.
  float y[3];
  for (int i = 0; i < 3; i++) {
    y[i] = output->data.f[i];
  }

  // Find the index of the maximum value
  int maxIndex = 0;
  for (int i = 1; i < 3; i++) {
    if (y[i] > y[maxIndex]) {
      maxIndex = i;
    }
  }

  // Output the detected posture
  Serial.println(classes[maxIndex]);

  // After posture detection, send the actual IMU readings over I2C to ESP32
  Wire.beginTransmission(8); // Replace 8 with the ESP32's I2C address
  Wire.write(reinterpret_cast<uint8_t*>(&x_acc), sizeof(float));
  Wire.write(reinterpret_cast<uint8_t*>(&y_acc), sizeof(float));
  Wire.write(reinterpret_cast<uint8_t*>(&z_acc), sizeof(float));
  Wire.write(reinterpret_cast<uint8_t*>(&x_gyro), sizeof(float));
  Wire.write(reinterpret_cast<uint8_t*>(&y_gyro), sizeof(float));
  Wire.write(reinterpret_cast<uint8_t*>(&z_gyro), sizeof(float));

  // Send the detected posture as a string
  Wire.write(static_cast<uint8_t>(maxIndex));

  Wire.endTransmission();

  // Optionally, add a delay if needed
  delay(3000);
}