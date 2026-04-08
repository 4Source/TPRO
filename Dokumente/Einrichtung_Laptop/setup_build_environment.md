# Setup Build Pipeline
## TODO: Group erstellen und ein Besitzer erstellen bevor installation!
## Install Dependencies

sudo apt-get install git wget flex bison gperf python3 python3-pip python3-venv cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0

## Install IDF 
mkdir -p /opt/esp/espressif_tools/python_env
sudo chown -R "username":esp-dev /opt/esp/espressif_tools
sudo chmod -R 775 /opt/esp/espressif_tools
cd /opt/esp
git clone -b v5.5 --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
export IDF_TOOLS_PATH="/opt/esp/espressif_tools"
export IDF_PYTHON_ENV_PATH="/opt/esp/espressif_tools/python_env"
./install.sh esp32s3

erstelle /opt/esp/activate_idf_v6.0.sh
chmod +x 

## Jetzt kann jeder User das script sourcen und idf verwenden
source /opt/esp/activate_idf_v6.0.sh

## Installation mit EIM
echo "deb [trusted=yes] https://dl.espressif.com/dl/eim/apt/ stable main" | sudo tee /etc/apt/sources.list.d/espressif.list

sudo apt update

sudo apt install eim

newgrp esp-dev
export IDF_TOOLS_PATH="/opt/esp/espressif_tools"
export IDF_PYTHON_ENV_PATH="/opt/esp/espressif_tools/python_env"

## Installer
eim wizard # Einfache Versionswahl - spannend für spätere Upgrades!

## Direkt install - env variablen in eim shell zwingen funktioniert nicht!
IDF_TOOLS_PATH="/opt/esp/espressif_tools" \
IDF_PYTHON_ENV_PATH="/opt/esp/espressif_tools/python_env" \
eim install --path /opt/esp/esp-idf --version-name v6.0 --target esp32s3

## Setup Aliases for global .bashrc - diese wird dann von jedem USer gesourced

/etc/profile.d/esp_dev.sh z.B. hier

### Alias für sourcen der Buildumgebung - Actung wenn idf unter /opt liegt bashrc anpassen

alias setup-esp=" . /home/CENTRAL_USER/esp/esp-idf/export.sh"
export IDF_PATH="/home/CENTRAL_USER/esp/esp-idf"
## Setup Project
idf.py create-project xyz

## CMAKE Strategie

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

## Erweiterungen
Alles über Tasks in .vscode/tasks.json lösen - Build, Flash, Debug

# Einrichten des Laptops
## Zentraler Speicherort für esp-idf

sudo mkdir -p /opt/esp
sudo groupadd esp-dev
sudo usermod -aG esp-dev "username"

# User ist Besitzer, Gruppe esp-dev hat Schreibrechte
sudo chown -R "username":esp-dev /opt/esp
sudo chmod -R 775 /opt/esp

# Neue Dateien erben die Gruppe esp-dev
sudo chmod g+s /opt/esp

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

## User erstellen
### Gruppe

sudo groupadd esp-dev

### Adden und Passwörter

sudo useradd -m -s /bin/bash "username" - Am besten wie Hochschulaccounts baurmo   
sudo usermod -a -G esp-dev,dialout "username"
sudo passwd "username" - Jeder selber

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

## Installation clang

. /opt/esp/activate_idf_v6.0.sh
python3 $IDF_PATH/tools/idf_tools.py install esp-clang

## .clang-files sind schon konfiguriert
## Bestenfalls Blink auf GH hochladen dann muss jeder nur noch die Extensions ins
