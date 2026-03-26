# Setup Build Pipeline
## Install Dependencies

sudo apt-get install git wget flex bison gperf python3 python3-pip python3-venv cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0

## Group erstellen und ein Besitzer erstellen bevor installation!

sudo groupadd esp-dev
sudo useradd -m -s /bin/bash "username" - Am besten wie Hochschulaccounts baurmo   
sudo usermod -a -G esp-dev,dialout "username"
sudo passwd "username" - Jeder selber

sudo mkdir -p /opt/esp/espressif_tools/python_env
sudo chown -R "username":esp-dev /opt/esp
sudo chmod -R 775 /opt/esp
sudo chmod g+s /opt/esp

sudo chown -R "username":esp-dev /opt/esp/espressif_tools
sudo chmod -R 775 /opt/esp/espressif_tools
cd /opt/esp
git clone -b v6.0 --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
export IDF_TOOLS_PATH="/opt/esp/espressif_tools"
export IDF_PYTHON_ENV_PATH="/opt/esp/espressif_tools/python_env"
./install.sh esp32s3

erstelle /opt/esp/activate_idf_v6.0.sh
chmod +x

## Git Repo von /opt/esp erstellen, falls die globale Installation zerschossen wird
## Möglicherweise müssen teile nachinstalliert werden - einfach mit Besitzer der Group den gegebenen Command ausführen wenn source fehlschlägt - Dann committen clang-tidy und format müssen auch nachinstalliert werden - auch extra commit bestenfalls

## Jetzt kann jeder User das script sourcen und idf verwenden
source /opt/esp/activate_idf_v6.0.sh

## Setup Project
idf.py create-project xyz

## CMAKE

Root-CMakeLists wird per idf erstellt
Jeder Folder bekommt eigene CMakeList für Modularität und Übersicht

## Build Prozess
- setup-esp
- idf.py set-target esp32s3 !!!Kann hier auch für linux Target kompiliert werden(tests webserver)???
- idf.py -B build-folder build

## Flashen
### Suchen des Ports(USB) mit ls /dev/ttyUSB* - Port per Umgebungsvariable vorgeben für Skripte??
### Berechtigung zum schreiben auf USB chmod 666
idf.py -B build-folder -p /dev/ttyUSB-ESP flash

## Lock Verzeichnis um glecihzeitigen Zugriff mehrerer User auf USB zu verhindern

sudo mkdir -p /var/lock/esp32
sudo chgrp esp-dev /var/lock/esp32
sudo chmod 775 /var/lock/esp32

## Lock Skript
/usr/local/bin/esp-lock

siehe BASH-Skript in Verzeichnis
sudo chmod +x /usr/local/bin/esp-lock

### Wenn ein Befehl auf USB schnittstelle kommt immer  ./esp-lock Skript als Argument
alias idf.py='esp-lock idf.py' # Immer locken vor idf.py call

### Entprechend auch die aliase der globalen bashrc anpassen

## Hostname setzen 

sudo hostnamectl set-hostname esp-build-server
- Ändern Eintrag 127.0.1.1 von name bisher zu esp-build-server in /etc/hosts oder einfach alten hostname nutzen
- Für Zugriff mit ssh username@esp-build-server

## Tailscale einrichten für zugriff von außerhalb des lokalen Netzes
### Installieren

curl -fsSL https://tailscale.com/install.sh | sh

### Anmelden - Ein Account wir erstellt über den können sich dann alle clients virtuell mit dem server im glecihen Netzwerk befinden

sudo tailscale up --ssh --accept-dns=true

### Einstellungen
#### Voraussetung - Jeder dev hat einen tailscale account, den tailscale client auf seinem rechner und den Einlandungslink
Share Server esp-build-server
Allow ssh access
Generiere Einladungslink - über den sollte jeder ins tailnet kommen


MagicDNS muss aktiviert sein für hostname auflösung von außerhalb

## Allgemeine Einstellungen um Laptop zum Server zu machen

In /etc/systemd/logind.conf für Ignorieren ddes zuklappens
HandleLidSwitch=ignore
HandleLidSwitchExternalPower=ignore
HandleLidSwitchDocked=ignore

sudo systemctl restart systemd-logind

Deaktivieren von Ruhemodus

sudo systemctl mask sleep.target suspend.target hibernate.target hybrid-sleep.target

## SSH Keys - Berechtgungen
chmod 700 ~/.ssh && chmod 600 ~/.ssh/authorized_keys

## VSCode
###Extensions:
- C/C++ Microsoft
- Task Buttons spencerwmiles

### COnfiguration

Übernehme den .vscode Ordner - möglicherweise müssen Pfade angepasst werden

## Installation clang

. /opt/esp/activate_idf_v6.0.sh
python3 $IDF_PATH/tools/idf_tools.py install esp-clang

## .clang-files und extensions sind schon konfiguriert
## Bestenfalls Blink auf GH hochladen dann muss jeder nur noch die Extensions remote installieren und kann alles konfiguriert verwenden
