# FreeRTOS Labs - OTA Example for AWS IoT Core

AWS IoT CoreOTA enables simple Over-The-Air (OTA) updates through an extensible
and modular set of interfaces. When properly setup, the CoreOTA 'Orchestrator'
(see CoreOTA concepts & architecture) will handle the standard OTA behaviors
like downloading the OTA file, parsing update metadata, and updating OTA status
remotely. These functions are performed through the different interfaces which
allow for customization for new fields and extensibility to new services. Using
this library will allow you to easily configure OTA updates from a variety of
sources and keep OTA functionality separated from any on-device application.

This repository contains an example of an OTA orchestrator using FreeRTOS and
coreMQTT. The example will check IoT Core for an existing OTA Job, download its
associated file, output the file over the command line, and report success back
to IoT Core.

## 0. Concepts and Architecture

A detailed breakdown of important concepts and architectural decisions can be
found [here](docs/design/CONCEPTS.md).

## 1. Demo Prerequisites

- A personal computer running a POSIX operating system (Linux, MacOS, or WSL)
  - (Optional) Use [the Nix package manager](https://nixos.org/download.html) to
    supply software dependencies
  - A C99 compiler (tested with Clang v11.1.0, GCC v9.4.0, and Apple clang
    v14.0.0)
  - CMake v3.16+ (tested with v3.24.3 and v3.26.1)
  - OpenSSL (tested with v1.1.1t and v1.1.1u)
- An
  [AWS account with with administrative access](https://docs.aws.amazon.com/iot/latest/developerguide/setting-up.html)
  to use for [AWS IoT Core](https://aws.amazon.com/iot-core/)

## 2. Demo Setup

### 2.1 Setup AWS IoT Core

To setup AWS IoT Core, follow the [AWS IoT Core Setup Guide](docs/AWSSetup.md).
The guide shows you how to sign up for an AWS account, create a user, and
register your device with AWS IoT Core. After you have followed the instructions
in the AWS IoT Core Setup Guide, you will have acquired the following items:

- A **device Endpoint**
- an AWS IoT **Thing** (and associated **ThingName**)
- **PEM-encoded device certificate**
- **PEM-encoded private key**
- **PEM-encoded root CA certificate**

The root CA certificate can also be downloaded
[here](https://www.amazontrust.com/repository/AmazonRootCA1.pem). The simulator
must be provided with these entities in order for it to connect with AWS IoT
Core.

### 2.2 Perform one-time setup of OTA cloud resources

Before you create an OTA job, the following resources are required. This is a
one time setup required for performing OTA firmware updates. Make a note of the
names of the resources you create, as you will need to provide them during
subsequent configuration steps.

- An Amazon S3 bucket to store your updated firmware. S3 is an AWS Service that
  enables you to store files in the cloud that can be accessed by you or other
  services. This is used by the OTA Update Manager Service to store the firmware
  image in an S3 "bucket" before sending it to the device.
  [Create an Amazon S3 Bucket to Store Your Update](https://docs.aws.amazon.com/freertos/latest/userguide/dg-ota-bucket.html).
- An OTA Update Service role. By default, the OTA Update Manager cloud service
  does not have permission to access the S3 bucket that will contain the
  firmware image. An OTA Service Role is required to allow the OTA Update
  Manager Service to read and write to the S3 bucket.
  [Create an OTA Update Service role](https://docs.aws.amazon.com/freertos/latest/userguide/create-service-role.html).
- An OTA user policy. An OTA User Policy is required to give your AWS account
  permissions to interact with the AWS services required for creating an OTA
  Update.
  [Create an OTA User Policy](https://docs.aws.amazon.com/freertos/latest/userguide/create-ota-user-policy.html).
- [Create a code-signing certificate](https://docs.aws.amazon.com/freertos/latest/userguide/ota-code-sign-cert-win.html).
  The demos support a code-signing certificate with an ECDSA P-256 key and
  SHA-256 hash to perform OTA updates.
- [Grant access to Code Signing for AWS IoT](https://docs.aws.amazon.com/freertos/latest/userguide/code-sign-policy.html).

### 2.3 Build the coreOTA_Demo Binary

On your personal computer, open a terminal and issue the following commands:

```bash
# optional command - skip if you do not have Nix installed
nix develop --extra-experimental-features "nix-command flakes"
```

```bash
mkdir build
cd build
cmake ..
make
```

## 3. Run the OTA Demo

### 3.1 Create an OTA Update in AWS IoT Core

Follow the steps to
[Create an OTA Update with the AWS IoT Core Console](https://docs.aws.amazon.com/freertos/latest/userguide/ota-console-workflow.html).
Apply the following options:

- When prompted, select the S3 bucket, OTA update service role, and code-signing
  profile you made in 2.2
- Under **Sign and choose your file**, select "Sign a new file for me".
- Under **Select the protocol for the file transfer**, select "MQTT"
- Under **Pathname of code signing certificate on device**, enter an arbitrary
  string (e.g. "test")

### 3.2 Run the simulator

After you've created your OTA update, start the simulator by using the following
command in your `build/` directory:

```
./coreOTA_Demo {certificateFilePath} {privateKeyFilePath} {rootCAFilePath} {endpoint} {thingName}
```

### 3.3 Verify successful OTA in the AWS IoT Core Console

After the simulator stops printing output, check the IoT Core Console to verify
that the OTA Job you created is marked as "Successful".

## 4. Run the Unit Tests

### 4.1 Running the OTA Parser Unit tests

The unit tests are broken out by the 'module' they reside in (aka the /lib
folder) and can be executed per module. To run the OTA parser unit tests you'll
run the following commands

From here you can build the unit tests...

```
cd lib/iot-core-jobs-ota-parser
cmake -B build -S test
make -C build
```

Once built, the test executables can be found under the
`lib/iot-core-jobs-ota-parser/build/bin/tests/` directory

## Security

See [CONTRIBUTING](CONTRIBUTING.md#security-issue-notifications) for more
information.

## License

This library is licensed under the MIT License.
