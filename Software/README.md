# About the project structure
```
Software/
├── .vscode/
├── build/ [auto generated]
├── components/ 
│   ├── network/
│   │   ├── include/ 
│   │   │   └── network.hpp
│   │   ├── CMakeLists.txt
│   │   ├── Kconfig
│   │   ├── network.cpp
│   │   └── README.md
│   └── webserver/
│       ├── include/ 
│       │   ├── embedded_files.hpp [auto generated]
│       │   ├── routes.hpp
│       │   └── webserver.hpp
│       ├── www/
│       │   ├── assets/
│       │   │   ├── app.js
│       │   │   └── index.css
│       │   └── index.html
│       ├── .gitignore
│       ├── CMakeLists.txt
│       ├── Kconfig
│       ├── README.md
│       ├── routes.cpp
│       └── webserver.cpp
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