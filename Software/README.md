# About the project structure
```
Software/
├── .vscode/
├── build/ [auto generated]
├── components/ 
│   ├── https_server/
│   │   ├── certs/ 
│   │   │   ├── generate.sh
│   │   │   ├── README.md
│   │   │   ├── server_cert.conf
│   │   │   ├── server_cert.pem [generated]
│   │   │   └── server_key.pem [generated]
│   │   ├── include/ 
│   │   │   ├── https_server.hpp
│   │   │   ├── restserver.hpp
│   │   │   └── websocket_server.hpp
│   │   ├── www/
│   │   │   ├── assets/
│   │   │   │   ├── app.js
│   │   │   │   └── index.css
│   │   │   └── index.html
│   │   ├── .gitignore
│   │   ├── CMakeLists.txt
│   │   ├── embedded_files.hpp [auto generated]
│   │   ├── filesserver.cpp
│   │   ├── filesserver.hpp
│   │   ├── https_server.cpp
│   │   ├── idf_component.yml
│   │   ├── Kconfig
│   │   ├── README.md
│   │   ├── restserver.cpp
│   │   ├── routes.cpp
│   │   ├── routes.hpp
│   │   ├── webserver.cpp
│   │   ├── webserver.hpp
│   │   └── websocket_server.cpp
│   └── network/
│       ├── include/ 
│       │   └── network.hpp
│       ├── CMakeLists.txt
│       ├── ethernet_network.cpp
│       ├── ethernet_network.hpp
│       ├── idf_component.yml
│       ├── Kconfig
│       ├── network.cpp
│       ├── README.md
│       ├── wifi_network.cpp
│       └── wifi_network.hpp
├── main/
│   ├── CMakeLists.txt
│   ├── Kconfig.projbuild
│   └── main.cpp
├── web/
│   ├── node_modules/
│   ├── public/
│   ├── src/
│   │   ├── assets/
│   │   ├── index.tsx
│   │   └── style.css
│   ├── index.html
│   ├── package.json
│   ├── README.md
│   ├── tsconfig.json
│   └── vite.config.ts
├── .clang-format
├── .clang-tidy
├── .gitignore
├── CMakeLists.txt
├── README.md [you are here]
├── sdkconfig [auto generated]
└── sdkconfig.defaults
```
This project structure is based on the example structure by esp-idf which you can find [here](https://docs.espressif.com/projects/esp-idf/en/v6.0/esp32s3/api-guides/build-system.html#example-project).

TODO: What is what for
## ``components/``
Contains the custom components for this project. 

# Hardware
https://www.waveshare.com/esp32-s3-eth.htm?sku=28972
https://www.waveshare.com/wiki/ESP32-S3-ETH

# googletest
Run when first repo clone
```git submodule update --init --recursive```