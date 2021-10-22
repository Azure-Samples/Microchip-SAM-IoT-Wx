# POC Notes

The max buffer size for the socket is 1400.

The size for the payload is hardcoded to a `uint8_t` which means max size is 255. Increased that to `uint16_t` to allow for larger payloads.

QOS 1 might be stagnating the sends

Malloc'ing a 2096 size buffer doesn't work out too great try and malloc the smaller one.
