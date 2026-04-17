#!/usr/bin/env bash
set -e

CERT_DIR="$(git rev-parse --show-toplevel)/Software/components/webserver/certs"
KEY="$CERT_DIR/server_key.pem"
CERT="$CERT_DIR/server_cert.pem"
CONF="$CERT_DIR/server_cert.conf"

mkdir -p "$CERT_DIR"

SDKCONFIG="$(git rev-parse --show-toplevel)/Software/sdkconfig"
if [ ! -f "$SDKCONFIG" ]; then
    echo "sdkconfig not found use sdkconfig.defaults instead"
    SDKCONFIG="$(git rev-parse --show-toplevel)/Software/sdkconfig.defaults"
fi
HOSTNAME=$(grep 'CONFIG_LWIP_LOCAL_HOSTNAME=' "$SDKCONFIG" | cut -d= -f2 | tr -d '"')
if [ -z "$HOSTNAME" ]; then
    echo "ERROR: CONFIG_LWIP_LOCAL_HOSTNAME not set"
    exit 1
fi
MDNS_NAME="${HOSTNAME}.local"

if [ -f "$KEY" ] && [ -f "$CERT" ]; then
    echo "Certificate already exists. Skipping."

    echo "Verifying certificate..."
    openssl verify -CAfile "$CERT" "$CERT"
    CERT_HASH=$(openssl x509 -noout -modulus -in "$CERT" | openssl md5);
    KEY_HASH=$(openssl rsa -noout -modulus -in "$KEY" | openssl md5);
    if [ "$CERT_HASH" != "$KEY_HASH" ]; then 
        echo 'ERROR: Key does not match certificate'; 
        rm -f "$KEY" "$CERT"
    elif ! openssl x509 -in "$CERT" -noout -ext subjectAltName | grep -q "DNS:$MDNS_NAME"; then
        echo 'ERROR: Hostname is not mentioned in certificate'; 
        rm -f "$KEY" "$CERT"
    else
        echo "Done."
        exit 0; 
    fi
fi

echo "Generating self-signed certificate..."

openssl req -x509 -newkey rsa:2048 \
    -keyout "$KEY" \
    -out "$CERT" \
    -days 3650 \
    -nodes \
    -config "$CONF" \
    -addext "subjectAltName=DNS:${MDNS_NAME},DNS:localhost"

echo "Verifying certificate..."
openssl verify -CAfile "$CERT" "$CERT"
CERT_HASH=$(openssl x509 -noout -modulus -in "$CERT" | openssl md5);
KEY_HASH=$(openssl rsa -noout -modulus -in "$KEY" | openssl md5);
if [ "$CERT_HASH" != "$KEY_HASH" ]; then 
        echo 'ERROR: Key does not match certificate'; 
        exit 1; 
fi

echo "Done."
