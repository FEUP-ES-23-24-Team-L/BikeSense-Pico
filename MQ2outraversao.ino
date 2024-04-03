#include <MQUnifiedSensor.h>

// Definir o PIN
const int pinSensor = A0;

MQUnifiedSensor MQ2(pinSensor, SENSOR_MQ2);

void setup() {

  Serial.begin(9600);
  MQ2.begin();
}

void loop() {
  //Ler os valores do sensor
  float ppm = MQ2.readSensor();
  
  // Imprimindo os valores lidos
  Serial.print("Concentração de gás: ");
  Serial.print(ppm);
  Serial.println(" ppm");

  //Intervalo entre leituras em milissegundos
  delay(500);
}