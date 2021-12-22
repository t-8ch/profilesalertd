# profilesalertd

Desktop notifications about changes to Linux platform\_profile.

Depends on the availability of the API-file `/sys/firmware/acpi/platform_profile`
and a notification daemon.

## How to build

```
meson build
ninja -C build
build/profilesalertd
```
