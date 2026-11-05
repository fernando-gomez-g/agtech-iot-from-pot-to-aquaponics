// Copy this file to secrets.h and fill in your own values.
// secrets.h is listed in .gitignore and must NEVER be committed.

#pragma once

// ---------- Wi-Fi (2.4 GHz network) ----------
#define WIFI_SSID     "your-wifi-network-name"
#define WIFI_PASSWORD "your-wifi-password"

// ---------- AWS IoT Core ----------
// Find it in IoT Core -> Settings -> Device data endpoint
#define AWS_IOT_ENDPOINT "xxxxxxxxxxxxxx-ats.iot.us-east-1.amazonaws.com"
#define THING_NAME       "menta-esp32"

// ---------- Certificates ----------
// Paste the full contents of each file, including the BEGIN/END lines.

// Amazon Root CA 1 (AmazonRootCA1.pem)
static const char AWS_CERT_CA[] = R"EOF(
-----BEGIN CERTIFICATE-----
PASTE_AMAZON_ROOT_CA_1_HERE
-----END CERTIFICATE-----
)EOF";

// Device certificate (xxxx-certificate.pem.crt)
static const char AWS_CERT_CRT[] = R"KEY(
-----BEGIN CERTIFICATE-----
PASTE_DEVICE_CERTIFICATE_HERE
-----END CERTIFICATE-----
)KEY";

// Device private key (xxxx-private.pem.key)
// Check the first line of your file: it may say "BEGIN RSA PRIVATE KEY"
// or "BEGIN PRIVATE KEY". Keep it exactly as it is in your file.
static const char AWS_CERT_PRIVATE[] = R"KEY(
-----BEGIN RSA PRIVATE KEY-----
PASTE_PRIVATE_KEY_HERE
-----END RSA PRIVATE KEY-----
)KEY";
