## Install pytest to your Environment
pip install pytest-embedded pytest-embedded-idf pytest-embedded-qemu
## Run all Tests
/usr/local/bin/esp-lock /dev/ttyACM0 bash -c 'pytest pytest/*.py --embedded-services esp,idf --target esp32s3'
## Run special Test
/usr/local/bin/esp-lock /dev/ttyACM0 bash -c 'pytest pytest/pytest_boot.py --embedded-services esp,idf --target esp32s3'
## Bitte immer Port locken wenn auf ESP getestet wird!