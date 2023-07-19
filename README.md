# AWS IoT CoreOTA (Embedded C)

AWS IoT CoreOTA enables simple Over-The-Air (OTA) updates through an extensible and modular set of interfaces. When properly setup, the CoreOTA 'Orchestrator' (see CoreOTA concepts & architecture) will
handle the standard OTA behaviors like downloadin the OTA file, parsing update metadata, and updating OTA status remotely. These functions are performed through the different interfaces which allow
for customization for new fields and extensibility to new services. Using this library will allow you to easily configure OTA updates from a variety of sources and keep OTA functionality
separated from any on-device application.

## CoreOTA Concepts and Archiecture
A detailed breakdown of important concepts and architectural decisions can be found [here](docs/design/CONCEPTS.md).

## Usage

```
nix build
./result certificateFilePath privateKeyFilePath rootCAFilePath endpoint thingName
```

or

```
nix develop
mkdir build
cd build
cmake ..
make
./coreOTA certificateFilePath privateKeyFilePath rootCAFilePath endpoint thingName
```

## Security

See [CONTRIBUTING](CONTRIBUTING.md#security-issue-notifications) for more
information.

## License

This library is licensed under the LICENSE NAME HERE License.
