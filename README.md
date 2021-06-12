# RoomPi

<p>
<img src="https://img.shields.io/badge/version-v7.0-success/"/>

<a href="https://github.com/margobra8/RoomPi/blob/main/LICENSE">
<img src="https://img.shields.io/badge/license-MIT-informational"/>
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

> Smart environmental monitoring system for buildings

> SDG2 Innovative project 2020-2021

##### Authors: Victoria M. Gullón y Marcos Gómez

## Introduction

RoomPi is a smart environmental monitoring system for use inside buildings. It is developed ontop Raspberry Pi 2B hardware.

![Imagen del Sistema RoomPi](https://drive.google.com/uc?id=1_iAIjsIHQ3ZLKhq2Y-fqx7Z6MbLN5vM4)

## System

The system makes use of the following sensors and actuators, as well as other devices:

- Sensors
  - DHT11 (temperature and humidity)
  - BH1750 (luxmeter _light intensity_)
  - CCS811 (equivalent CO2)
- Actuators
  - HD44780 (2-line dot matrix character display)
  - SN74HC595 (8 bit shift register for LED strip)
  - Active buzzer
- Control
  - 3 buttons
- Others
  - LM117 voltage regulator

## Executable compilation

| :warning: _Before attempting to compile_ you must have the required [dependencies](#dependencias) installed |
| :--- |

The project makes use of the following libraries:

- wiringPi
- pthread
- rt
- libcurl

When compiling you must specify the libraries used:

```sh
gcc src/*.c src/sensors/*.c src/actuators/*. src/libs/*.c src/controllers/*.c -lpthread -lrt -lwiringPi -lcurl -o "roompi-bin"
```

For cross compilation from Eclipse you will need to install the Raspbian armhf toolchain.

## Web subsystem ([Docker](https://docs.docker.com/get-started/overview/))

The web subsystem comprises the following elements:

- [InfluxDB](https://docs.influxdata.com/influxdb/v2.0/) Database
- [Grafana](https://grafana.com/tutorials/grafana-fundamentals/?pg=docs) dashboard
- DB Manager [Chronograf](https://docs.influxdata.com/chronograf/v1.8/)
- Configuration application written in [Flask](https://flask.palletsprojects.com/en/2.0.x/)

These all run as containerized instances which are orchestrated by the [Docker service](https://docs.docker.com/get-started/overview/#the-docker-daemon).

To configure and launch the instances you must [install Docker](https://docs.docker.com/engine/install/debian/#install-using-the-convenience-script) and [docker-compose](https://docs.docker.com/compose/install/) in the host machine and edit the `.env` file where the desired username and password for Grafana and InfluxDB are specified.

Then you can bring up all instances by executing the following command from the root directory of the project

```sh
docker-compose up -d
```

You can stop all services by executing the next command (to remove the [persistent volumes](https://docs.docker.com/storage/volumes/) created in the process append `-v` to the command)

```sh
docker-compose down -d
```

The containers are configured to restart automatically in case they fail as well as start up automatically when the OS boots.

## Dependencies

If working directly on the project's source code, you must take into account the following dependencies and libraries:

### C Compiler

The C compiler must compile for the arm32v7 architecture used by the BCM2836 IC. To accomplish this you need to install the following dependencies on Linux:

```sh
sudo apt-get install -y libc6-armel-cross libc6-dev-armel-cross binutils-arm-linux-gnueabi libncurses5-dev build-essential bison flex libssl-dev gcc-arm-linux-gnueabihf
```

### C Libraries

To make use of the mentioned libraries in the project you must compile and install them beforehand. To dowload the various libraries you need have `git` installed.

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

### Libraries for the configuration web app with Flask in Python

To work on the custom web app used to configure the system, you need to install the following dependencies

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

It is highly recommended that you use a Python virtual environment when developing the project. Python's package manager pip is installed automatically in the virtual environment. Navigate instide the `flask-config-roompi` directory and run:

- venv

```sh
python3.8 -m venv venv
source venv/bin/activate
```

- Flask and other dependencies

```sh
pip install -r requirements.txt
```

[jQuery](https://api.jquery.com/) and [Bootstrap](https://getbootstrap.com/docs/5.0/getting-started/introduction/) are used for the web app UI.
