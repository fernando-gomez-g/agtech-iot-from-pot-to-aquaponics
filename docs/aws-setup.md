# AWS setup

Reproducible steps for the cloud side of the project. All resources were created in `us-east-1`. Replace `YOUR-BUCKET` and `ACCOUNT_ID` with your own values, and keep real account IDs out of anything you publish.

## 1. AWS IoT Core: device, certificate and policy

1. IoT Core -> Manage -> Things -> Create single thing. Name: `menta-esp32`.
2. Auto-generate a new certificate and download the device certificate, private key and Amazon Root CA 1. They can only be downloaded once. Store them outside the repository.
3. Create and attach a policy. A tighter policy, scoped to this device and topic, is recommended:

```json
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": "iot:Connect",
      "Resource": "arn:aws:iot:us-east-1:ACCOUNT_ID:client/menta-esp32"
    },
    {
      "Effect": "Allow",
      "Action": "iot:Publish",
      "Resource": "arn:aws:iot:us-east-1:ACCOUNT_ID:topic/menta/sensores"
    }
  ]
}
```

The early prototype used a broad policy (`Resource: "*"` with connect, publish, subscribe and receive). The tighter version above has not yet been applied to the running prototype.

4. Copy the device data endpoint from IoT Core -> Settings.

Verify messages arrive with IoT Core -> MQTT test client, subscribed to `menta/sensores`.

## 2. Cold path: Firehose -> S3 -> Athena

### S3 bucket

Create a private bucket (for example `menta-datalake-xx`) in the same region. Keep Block Public Access on, ACLs disabled, default encryption. Versioning is not needed because the data is append-only.

### Firehose stream

- Source: Direct PUT. Destination: Amazon S3. Name: `menta-to-s3`.
- Prefix: `lecturas/year=!{timestamp:yyyy}/month=!{timestamp:MM}/day=!{timestamp:dd}/`
- Error prefix: `errores/!{firehose:error-output-type}/year=!{timestamp:yyyy}/month=!{timestamp:MM}/day=!{timestamp:dd}/`
- Buffer interval: 60 seconds while testing, then 300 seconds or more to avoid many tiny files.

### IoT rule `menta_to_firehose`

```sql
SELECT *, clientid() AS device_id, timestamp() AS ts FROM 'menta/sensores'
```

Action: Amazon Data Firehose -> `menta-to-s3`, separator **newline**. The separator matters because Firehose concatenates records, and Athena needs one JSON object per line.

### Athena

Set the query result location to `s3://YOUR-BUCKET/athena-resultados/` (Athena -> Settings), then:

```sql
CREATE DATABASE menta_db;

CREATE EXTERNAL TABLE menta_db.lecturas (
  humedad_suelo int,
  temperatura double,
  humedad_aire double,
  device_id string,
  ts bigint
)
PARTITIONED BY (year string, month string, day string)
ROW FORMAT SERDE 'org.openx.data.jsonserde.JsonSerDe'
LOCATION 's3://YOUR-BUCKET/lecturas/'
TBLPROPERTIES (
  'projection.enabled'='true',
  'projection.year.type'='integer',
  'projection.year.range'='2026,2035',
  'projection.month.type'='integer',
  'projection.month.range'='1,12',
  'projection.month.digits'='2',
  'projection.day.type'='integer',
  'projection.day.range'='1,31',
  'projection.day.digits'='2',
  'storage.location.template'='s3://YOUR-BUCKET/lecturas/year=${year}/month=${month}/day=${day}/'
);
```

`ts` is epoch milliseconds in UTC. Convert with `from_unixtime(ts/1000, 'America/Mexico_City')`.

## 3. Hot path: CloudWatch metrics, dashboard and alarms

### IoT rule `menta_to_cloudwatch`

```sql
SELECT * FROM 'menta/sensores'
```

One CloudWatch metric action per sensor, namespace `Menta`:

| Metric name | Unit | Value |
|---|---|---|
| `humedad_suelo` | Percent | `${humedad_suelo}` |
| `humedad_aire` | Percent | `${humedad_aire}` |
| `temperatura` | None | `${temperatura}` |

The IoT CloudWatch action does not support metric dimensions. For multiple devices, publish metrics from a Lambda function instead so they can be tagged per device.

### Dashboard

Line widgets with statistic Average and a 1-minute period: one for soil and air humidity (same unit), one for temperature.

### Alarms (notification through an SNS topic with a confirmed email subscription)

| Alarm | Metric and statistic | Condition | Missing data |
|---|---|---|---|
| Soil too dry | `humedad_suelo`, Average, 5 min | Lower than threshold, 3 of 3 datapoints | Treat as missing |
| No data received | `humedad_suelo`, SampleCount, 5 min | Lower than 1, 2 of 2 datapoints | Treat as breaching |

Add an OK notification to the same SNS topic if you want a recovery message. To test an alarm, temporarily raise (or lower) its threshold, wait for the email, then restore it.

## 4. Cost control

- Configure an AWS Budgets alert before creating anything.
- Reading once per minute keeps message volume low.
- Delete or stop resources you no longer use.
