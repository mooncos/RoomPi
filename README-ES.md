# RoomPi

<p>
<img src="https://img.shields.io/badge/version-v7.0-success/"/>

<a href="https://github.com/margobra8/RoomPi/blob/main/LICENSE">
<img src="https://img.shields.io/github/license/margobra8/RoomPi"/>
</a>

<a href="https://github.com/margobra8/RoomPi/tree/main/docs">
<img src="https://img.shields.io/badge/docs-.pdf-informational"/>
</a>

<a href="https://api.codetabs.com/v1/loc/?github=margobra8/RoomPi">
<img src="https://img.shields.io/badge/dynamic/json?color=9dc&label=lines%20of%20code&query=%24%5B-1%3A%5D.linesOfCode&url=https%3A%2F%2Fapi.codetabs.com%2Fv1%2Floc%2F%3Fgithub%3Dmargobra8%2FRoomPi"/>
</a>
<a href="https://github.com/margobra8/RoomPi">
<img src="https://img.shields.io/github/languages/top/margobra8/RoomPi?logo=github">
</a>
</p>

> Sistema de monitorización ambiental para interiores
> Proyecto Innovador SDG2 curso 2020-2021

##### Autores: Victoria M. Gullón y Marcos Gómez

## Introducción

RoomPi es un sistema de monitorización de las condiciones ambientales en interiores. Está desarrollado sobre el hardware RaspberryPi 2B.

![Imagen del Sistema RoomPi](https://drive.google.com/uc?id=1_iAIjsIHQ3ZLKhq2Y-fqx7Z6MbLN5vM4)

## Sistema

Actualmente el sistema hace uso de los siguientes sensores y actuadores así como otros dispositivos:

- Sensores
  - DHT11 (temperatura y humedad)
  - BH1750 (luxómetro)
  - CCS811 (CO2 equivalente)
- Actuadores
  - HD44780 (pantalla de caracteres de dos líneas)
  - SN74HC595 (shift register de 8 bits para tira de LEDs)
  - Buzzer activo
- Control
  - 3 botones en la parte frontal
- Adicional
  - Regulador de tensión LM117

## Compilación del ejecutable

| :warning: _Antes de compilar_ es necesario tener todas las [librerías y dependencias](#dependencias) instaladas |
| :--- |

El proyecto hace uso de las siguentes librerías:

- wiringPi
- pthread
- rt
- libcurl

Para compilar es necesario hacerlo con las librerias especificadas, en Raspbian:

```sh
gcc src/*.c src/sensors/*.c src/actuators/*. src/libs/*.c src/controllers/*.c -lpthread -lrt -lwiringPi -lcurl -o "roompi-bin"
```

Para cross compile en Eclipse instalar la toolchain para Raspbian armhf y compilar desde Eclipse.

## Subsistema web ([Docker](https://docs.docker.com/get-started/overview/))

El subsistema web está compuesto por los siguientes elementos:

- BBDD [InfluxDB](https://docs.influxdata.com/influxdb/v2.0/)
- Tablero [Grafana](https://grafana.com/tutorials/grafana-fundamentals/?pg=docs)
- Gestor de BBDD [Chronograf](https://docs.influxdata.com/chronograf/v1.8/)
- Aplicación de configuración en [Flask](https://flask.palletsprojects.com/en/2.0.x/)

Todos estos sistemas funcionan como contenedores y están orquestrados por el [servicio Docker](https://docs.docker.com/get-started/overview/#the-docker-daemon).

Para configurar y arrancar los proyectos es necesario tener [instalados Docker](https://docs.docker.com/engine/install/debian/#install-using-the-convenience-script) y [docker-compose](https://docs.docker.com/compose/install/) en el sistema host y editar el archivo `.env` con el usuario y contraseñas desados para la instancia de Grafana e InfluxDB.

Después, en la raíz del proyecto se debe ejecutar

```sh
docker-compose up -d
```

Para detener todos los servicios se puede ejecutar lo siguiente (añadir `-v` al comando para también eliminar los [volúmenes persistentes](https://docs.docker.com/storage/volumes/) que se hubiesen creado)

```sh
docker-compose down -d
```

Los contenedores están configurados para reiniciarse en caso de tener algún fallo e iniciarse automáticamente al arrancar el sistema.

## Dependencias

En caso de querer trabajar sobre el código fuente del proyecto se deben tener en cuenta las siguientes dependencias utilizadas:

### Compilador C

El compilador C debe compilar para la arquitectura arm32v7 que utiliza el chip BCM2836, para ello se necesitan instalar las siguientes dependencias en Linux

```sh
sudo apt-get install -y libc6-armel-cross libc6-dev-armel-cross binutils-arm-linux-gnueabi libncurses5-dev build-essential bison flex libssl-dev gcc-arm-linux-gnueabihf
```

### Librerías para el ejecutable en C

Para instalar las librerías utilizadas en el proyecto es necesario compilar e instalar dichas dependencias. Las librerías necesitan tener `git` para descargarse.

- wiringPi

```sh
git clone https://github.com/WiringPi/WiringPi
cd WiringPi
./build
```

- libcurl

```sh
sudo apt-get install -y autotools-dev autoconf libtool
git clone https://github.com/curl/curl
cd curl/
autoreconf -fi
./configure --without-ssl
sudo make install
```

### Librerías para la app web de ajuste con Flask en Python

Para desarrollar sobre la aplicación web personalizada para ajustar los parámetros del sistema se necesita instalar las siguientes dependencias

- Python 3.8

```sh
sudo apt-get install -y checkinstall libreadline-gplv2-dev libncursesw5-dev libssl-dev build-essential
cd /opt
sudo wget https://python.org/ftp/python/3.8.7/Python-3.8.7.tgz
sudo tar xzf Python-3.8.7.tgz
cd Python-3.8.7
sudo ./configure --enable-optimizations
sudo make altinstall # para evitar reemplazar el binario Python del sistema
python3.8 -V
```

Para el desarrollo web se recomienda el uso de un entorno virtual de Python. El gestor de paquetes de Python pip se instala automáticamente en el entorno virtual. En la carpeta `flask-config-roompi` del proyecto web:

- venv

```sh
python3.8 -m venv venv
source venv/bin/activate
```

- Flask y demás dependencias

```sh
pip install -r requirements.txt
```

La aplicación web utiliza [jQuery](https://api.jquery.com/) y [Bootstrap](https://getbootstrap.com/docs/5.0/getting-started/introduction/) para la interfaz de usuario.
