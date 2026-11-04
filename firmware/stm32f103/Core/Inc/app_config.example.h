#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/* Replace the placeholder values before flashing the firmware. */
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

#define ONENET_PRODUCT_ID "YOUR_PRODUCT_ID"
#define ONENET_DEVICE_NAME "YOUR_DEVICE_NAME"
#define ONENET_TOKEN "YOUR_DEVICE_TOKEN"
#define ONENET_HOST ONENET_PRODUCT_ID ".mqtts.acc.cmcconenet.cn"
#define ONENET_PORT "1883"

/* Calibrate this value for the feeder mechanism. */
#define FEEDER_STEPS_PER_GRAM 50

#endif /* APP_CONFIG_H */
