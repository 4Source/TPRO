# Setup Laptop
## Install Dependencies
```console
sudo apt-get install git wget flex bison gperf python3 python3-pip python3-venv cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0
```


## Group erstellen und ein Besitzer erstellen bevor installation!
```console
sudo groupadd esp-dev
```
### Add User  - Am besten wie Hochschulaccounts baurmo  
```console
sudo useradd -m -s /bin/bash "username"
sudo usermod -a -G esp-dev,dialout "username"
sudo passwd "username"
```

```console
sudo mkdir -p /opt/esp/espressif_tools/python_env
sudo chown -R "username":esp-dev /opt/esp
sudo chmod -R 775 /opt/esp
sudo chmod g+s /opt/esp
sudo chown -R "username":esp-dev /opt/esp/espressif_tools
sudo chmod -R 775 /opt/esp/espressif_tools
```
### Mit User der Gruppe
```console
cd /opt/esp
git clone -b v6.0 --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
export IDF_TOOLS_PATH="/opt/esp/espressif_tools"
export IDF_PYTHON_ENV_PATH="/opt/esp/espressif_tools/python_env"
./install.sh esp32s3
```
### Source Skript
```console
touch /opt/esp/activate_idf_v6.0.sh
chmod +x
```

### Git Repo von /opt/esp erstellen, falls die globale Installation zerschossen wird
#### Möglicherweise müssen Teile nachinstalliert werden - einfach mit Besitzer der Group den gegebenen Command ausführen wenn source fehlschlägt - Dann committen clang-tidy und format müssen auch nachinstalliert werden - auch extra commit bestenfalls

## Jetzt kann jeder User das script sourcen und idf verwenden
```console
source /opt/esp/activate_idf_v6.0.sh
```

## Setup Project
```console
idf.py create-project xyz
```

## CMAKE

Root-CMakeLists wird per idf erstellt
Jeder Folder bekommt eigene CMakeList für Modularität und Übersicht

## Build Prozess
```console
- idf.py set-target esp32s3
- idf.py -B build-folder build
```

## Flashen
### Suchen des Ports(USB) mit ls /dev/ttyUSB* - Port per Umgebungsvariable vorgeben für Skripte??
### Berechtigung zum schreiben auf USB chmod 666
```console
idf.py -B build-folder -p /dev/ttyUSB-ESP flash
```

## Lock Verzeichnis um gleichzeitigen Zugriff mehrerer User auf USB zu verhindern
```console
sudo mkdir -p /var/lock/esp32
sudo chgrp esp-dev /var/lock/esp32
sudo chmod 775 /var/lock/esp32
```
## Lock Skript - siehe BASH-Skript in Verzeichnis
```console
/usr/local/bin/esp-lock
```

```console
sudo chmod +x /usr/local/bin/esp-lock
```

### Wenn ein Befehl auf USB schnittstelle kommt immer  ./esp-lock Skript als Argument
```console
alias idf.py='esp-lock idf.py' # Immer locken vor idf.py call nur wenn flash und monitor
```

### Entprechend auch die aliase der globalen bashrc anpassen - in activate Skript


## Zugriff mit ssh username@esp-build-server

### Tailscale einrichten für zugriff von außerhalb des lokalen Netzes
### Installieren
```console
curl -fsSL https://tailscale.com/install.sh | sh
```

### Account erstellen und tailscale registieren
```console
sudo tailscale up --ssh --accept-dns=true
```

### Einstellungen
### Voraussetung - Jeder dev hat einen tailscale account, den tailscale client auf seinem rechner und den Einlandungslink
Share Server esp-build-server  
Allow ssh access
Generiere Einladungslink - über den sollte jeder ins tailnet kommen
### Wichtig! - In Tailscale JSON SSH User freigeben
	"ssh": [
		{
			"src": [
				"autogroup:member",
				"tag:tagname",
				"moelde01@github",
				"stefanzglr@github",
				"PC-512@github",
				"IbacJa@github",
				"BRKLNBB@github",
			],
			"dst":    ["tag:tagname"],
			"users":  ["autogroup:nonroot", "root"],
			"action": "accept",
		},
	],


## Allgemeine Einstellungen um Laptop zum Server zu machen

### Deaktivieren von Ruhemodus
```console
sudo systemctl mask sleep.target suspend.target hibernate.target hybrid-sleep.target
```

## SSH Keys - Berechtgungen
```console
chmod 700 ~/.ssh && chmod 600 ~/.ssh/authorized_keys
```

## VSCode
### Extensions:
- C/C++ Microsoft
- Task Buttons spencerwmiles

### Configuration

Übernehme den .vscode Ordner aus Repo

## Installation clang
```console
. /opt/esp/activate_idf_v6.0.sh
python3 $IDF_PATH/tools/idf_tools.py install esp-clang
```

### .clang-files und extensions sind schon konfiguriert
### Bestenfalls Blink auf GH hochladen dann muss jeder nur noch die Extensions remote installieren und kann alles konfiguriert verwenden
## Gitlab Runner
gitlab-runner User erstellen sudo, esp-dev und dialout gruppe
### Gitlab Runner installieren
```console
curl -L "https://packages.gitlab.com/install/repositories/runner/gitlab-runner/script.deb.sh" -o script.deb.sh
sudo bash script.deb.sh
sudo apt install gitlab-runner
```
### Runner registrieren
```console
sudo gitlab-runner register
```

## Web development
### Install node 22.x and npm globally 
```console
curl -fsSL https://deb.nodesource.com/setup_22.x | sudo -E bash -
sudo apt install -y nodejs
```

## Setup Debugger
Based on the [ESP-IDF JTAG Debugging](https://docs.espressif.com/projects/esp-idf/en/v6.0.1/esp32s3/api-guides/jtag-debugging/)

### OpenOCD
OpenOCD should already be installed 
```console
openocd --version
```

In the `/etc/udev/rules.d` folder add the following files:
- `60-openocd.rules` with the content from [here](https://github.com/espressif/openocd-esp32/blob/master/contrib/60-openocd.rules)
- `70-esp32s3-jtag.rules` with 
```
ATTRS{idVendor}=="303a", ATTRS{idProduct}=="1001", MODE="0660", GROUP="esp-dev", TAG+="uaccess"
```

Refresh udev rules with the command
```console
sudo udevadm control --reload-rules && sudo udevadm trigger
```

After that OpenOCD should be possible to start without any errors 
```console
openocd -f board/esp32s3-builtin.cfg
```
