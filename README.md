# RoomPi
Proyecto Innovador SDG2 curso 2020-2021

*Autores: Victoria M. Gullón y Marcos Gómez*

## Introducción
RoomPi es un sistema de monitorización de las condiciones ambientales de un aula. Está desarrollado sobre el hardware RaspberryPi 2B.

![Imagen del Sistema RoomPi](https://i.imgur.com/fkiQ8cR.jpg)

Actualmente el sistema hace uso de los siguientes sensores y actuadores externos:

- Sensores
  - DHT11 (temperatura y humedad)
  - BH1750 (luxómetro)
- Actuadores
  - HD44780 (pantalla de caracteres de dos líneas)
  - SN74HC595 (shift register de 8 bits para tira de LEDs)
  - Buzzer activo

El proyecto hace uso de las siguentes librerías:

- wiringPi
- pthread
- rt

Para compilar es necesario hacerlo con las librerias especificadas, en Raspbian:

```
gcc src/*.c src/sensors/*.c src/actuators/*. src/libs/*.c src/controllers/*.c -lpthread -lrt -lwiringPi -o "roompi-bin"
```

Para cross compile en Eclipse instalar la toolchain para Raspbian armhf y compilar desde Eclipse.
