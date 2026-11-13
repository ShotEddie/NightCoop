# NightCoop Hub on Windows

All Windows related files are located in the folder `tools/hub/windows/`. The hub itself is developed by [Twenfys](https://github.com/Twenfys/nightcoop-hub).

## Build and run

1. If you know how to compile a C++ file that's great! It will make things easier, for both of us.. If you don't know how to compile a C++ file just run the release file from a folder where you know you won't lose track of it.

## Using the panel

- `1` — Opens the required UDP ports in Windows Firewall and starts the hub
- `0` — Stops the hub and closes the ports
- `2` — Detects your public IP and copies the `IP#fingerprint` address to the clipboard
- `x` — Closes the panel. If the hub is running, it continues running in the background

Give other players your `IP#fingerprint` address. They can connect from the game by going to **Co-op → Server → Add an own server**.

If your VPS provider has a firewall you also need to allow UDP ports 47800 and 47810 to 47873 there. I checked this on my setup and forgetting to open these ports caused connection issues. Make sure to double-check the firewall settings, on the provider’s side not just your server. It’s easy to miss if you’re used to managing the server firewall.
## Files and settings

The `data` folder, located next to the executable, stores the key, logs, and extracted hub files.

Settings are stored in `NightCoopHub.ini`, which is created on the first run:

```ini
name=my-hub
region=
port=47800
shards=auto
max_clients=512
```

To move the hub to another machine without changing its key or fingerprint, copy the entire folder, including `data`. If you leave out `data`, the hub generates a new key.

## Licensing

- The hub repository does not publish a license. I advise not distributing NightCoopHub.exe because NightCoopHub.exe contains the hub. Build NightCoopHub.exe on the machine where you plan to run it.

- The files, in tools/hub/windows/ are licensed under GPL-3.0. Check the LICENSE file located in that folder. This GPL-3.0 license applies to those files.
