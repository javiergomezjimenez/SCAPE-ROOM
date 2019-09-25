
#include <Stepper.h>

const int stepsPerRevolution = 400;  // ÁNGULO JUSTO DE APERTURA DE LA PUERTA

// initialize the stepper library on pins 8 through 11:
Stepper myStepper(stepsPerRevolution, 8, 10, 9, 11);
int estado = 0;

void setup() {
  // set the speed at 20 rpm:
  myStepper.setSpeed(10);
  // initialize the serial port:
  Serial.begin(9600);
  myStepper.step(stepsPerRevolution); //ABRIR
  pinMode (7,INPUT);
  delay(500);
}

void loop() {
  Serial.println(estado);

  switch(estado){     //CADA ESTADO SERÁ UN COLOR
                case 0:     //SE CIERRA A LOS 30s
                      delay(30000);
                      myStepper.step(-stepsPerRevolution); //CERRAR
                      delay(500);
                      estado++;
                    break;
                case 1:     // NO HACE NADA
                    break;
                case 2:     // SE ABRE CUANDO LA CONTRASEÑA ES CORRECTA
                    myStepper.step(stepsPerRevolution); //ABRIR
                    estado++;
                    delay(30000); //SE CIERRA A LOS 30s
                    myStepper.step(-stepsPerRevolution); //CERRAR
                    break;
                case 3:     // NO HACE NADA FIN SCAPE ROOM
                    break;
                }
  if(digitalRead(7)&&estado<2)
  {
    estado = 2;
  }
  Serial.println(digitalRead(7));
}

