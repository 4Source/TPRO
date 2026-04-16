#!/usr/bin/env bash
set -e

CERT_DIR="$(git rev-parse --show-toplevel)/Software/components/webserver/certs"
KEY="$CERT_DIR/server_key.pem"
CERT="$CERT_DIR/server_cert.pem"
CONF="$CERT_DIR/server_cert.conf"

mkdir -p "$CERT_DIR"

if [ -f "$KEY" ] && [ -f "$CERT" ]; then
    echo "Certificate already exists. Skipping."

    echo "Verifying certificate..."
    openssl verify -CAfile "$CERT" "$CERT"
    CERT_HASH=$(openssl x509 -noout -modulus -in "$CERT" | openssl md5);
    KEY_HASH=$(openssl rsa -noout -modulus -in "$KEY" | openssl md5);
    if [ "$CERT_HASH" != "$KEY_HASH" ]; then 
        echo 'ERROR: Key does not match certificate'; 
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
    -config "$CONF"

echo "Verifying certificate..."
openssl verify -CAfile "$CERT" "$CERT"
CERT_HASH=$(openssl x509 -noout -modulus -in "$CERT" | openssl md5);
KEY_HASH=$(openssl rsa -noout -modulus -in "$KEY" | openssl md5);
if [ "$CERT_HASH" != "$KEY_HASH" ]; then 
        echo 'ERROR: Key does not match certificate'; 
        exit 1; 
fi

echo "Done."
