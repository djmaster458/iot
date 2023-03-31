#ifndef SECRETS_H
#define SECRETS_H
/*
    Define your WiFi config and AWS Credentials
*/

#include <pgmspace.h>

const char *AWS_IOT_ENDPOINT = "";
const char *THING_NAME = "";

const char *WIFI_SSID = "";
const char *WIFI_PASS = "";

static const char AWS_CERT_CA[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
-----END CERTIFICATE-----
)EOF";

static const char AWS_CERT_CRT[] PROGMEM = R"KEY(
-----BEGIN CERTIFICATE-----
-----END CERTIFICATE-----
)KEY";

static const char AWS_CERT_PRIVATE[] PROGMEM = R"KEY(
-----BEGIN RSA PRIVATE KEY-----
-----END RSA PRIVATE KEY-----
)KEY";

#endif