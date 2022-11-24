# Wordclock IR Gateway

> Firmware for our combined [Word Clock](https://wiki.netz39.de/projects:2019:wordclock) and [MQTT IR Gateway](https://wiki.netz39.de/projects:2019:mqtt-ir-gateway)


## Building the firmware

Install platformio with

```bash
pip3 install --user platformio
```

Or pick any other installation method from https://docs.platformio.org/en/latest/core/installation.html

Review https://docs.platformio.org/en/latest//faq.html#platformio-udev-rules and install the udev rules.

To refresh your groups inside a running session without logging in, you can use:

```bash
su - $USER
```

After doing this (or a reboot), you can then flash the firmware with:

```bash
platformio run -t upload
```

Please refer to the [PlatformIO documentation](https://docs.platformio.org/en/latest/core/index.html) for more details and alternative build options.

## License

[MIT](LICENSE.txt) © 2022 Netz39 e.V. and contributors
