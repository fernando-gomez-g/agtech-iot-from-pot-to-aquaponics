# Troubleshooting log

Problems found while building the project and how they were solved.

## Hardware and toolchain

| Problem | Cause | Solution |
|---|---|---|
| The ESP32 is too wide and leaves no free breadboard holes | Standard breadboards are narrow for a 38-pin board | Straddle two breadboards so each side keeps free columns |
| Arduino IDE shows the port greyed out | Windows has no driver for the USB-serial chip | Identify the chip (CP2102 here) and install the matching driver |
| Upload hangs on "Connecting......" | The board is not in download mode | Hold the BOOT button when "Connecting" appears, release when writing starts |
| Error "Wrong boot mode detected (0x17)" | Same as above, timing missed | Repeat the BOOT sequence; if needed hold BOOT, tap EN/RST, then upload |
| Blink on pin 2 shows nothing | This board has no user LED on pin 2 | Verify with `Serial.println` and the Serial Monitor instead |
| Serial Monitor is empty after upload | The program ran once in setup before the monitor opened | Press EN/RST with the monitor open at 115200 baud |
| Compile error "stray '#' in program" | Two `#define` lines pasted onto one line | Put each `#define` on its own line |
| DHT22 prints errors for the first readings | The sensor needs a moment after power-up | Expected; the code skips failed reads and retries |
| Wi-Fi connects only after many retries | Both bands share one network name and the ESP32 only supports 2.4 GHz | Give the 2.4 GHz band its own name (or use a phone hotspot while testing) |

## AWS

| Problem | Cause | Solution |
|---|---|---|
| Endpoint not visible under Settings | Console layout varies | Look under IoT Core settings or the Connect wizard |
| Timestream for LiveAnalytics not available | Closed to new customers on June 20, 2025 | Use S3 + Athena (or Timestream for InfluxDB) |
| Amazon Managed Grafana cannot be set up | Requires IAM Identity Center | Use CloudWatch dashboards, or self-hosted Grafana |
| Athena cannot parse the Firehose files | Records are concatenated without separators | Set the newline separator in the IoT rule's Firehose action |
| CloudWatch alarm for missing data never fires | Missing datapoints are ignored by default | Use SampleCount and set missing data treatment to "breaching" |
| Typo in a rule name (`mentao_to_cloudwatch`) | IoT rules and CloudWatch alarms cannot be renamed | Kept as is; recreating would only change the label. Documentation uses the intended name `menta_to_cloudwatch` |

## Calibration notes

- The soil sensor reads about 3200 in air and about 900 in water. The percentage is a relative scale, not true volumetric water content.
- For a better calibration, record the raw value in completely dry soil and right after thorough watering, then adjust the two constants.
- Publishing the raw value in the payload allows recalibration afterwards in SQL without losing past data.
