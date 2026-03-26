#!/usr/bin/env bash

export IDF_PATH="/opt/esp/esp-idf"
export IDF_TOOLS_PATH="/opt/esp/espressif_tools"
export IDF_PYTHON_ENV_PATH="/opt/esp/espressif_tools/python_env"

if [ -f "$IDF_PATH/export.sh" ]; then
    . "$IDF_PATH/export.sh" --targets esp32s3
    echo "INFO:         Benutzer: $(whoami)"
    echo "INFO:         IDF_PATH: $IDF_PATH"
else
    echo "ERROR:        Konnte $IDF_PATH/export.sh nicht finden"
fi
