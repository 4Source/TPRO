# Overview
This project uses a script (`generate.sh`) to generate a **self-signed TLS certificate** and a private key for a HTTPS server. The script is optimized for development so that user could try generating multiple times if certificate and private key are available and valid the script will skip generation of new certificate and private key. This allows to install the certificate once (till user generates new). 

# Installation 
To install the certificate save the `server_cert.pem` on your local machine form which you would like to access the Website. 

For Windows user the `server_cert.pem` has to be renamed to `server_cert.crt`. 

Than you can install the certificate as for you OS is usual. After this you should be able to access the Website with https with a secure connection. 