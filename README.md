# re_nsyshid — Web Toy Pad

Control the emulated **LEGO Dimensions Toy Pad** from your phone or PC while you play on Wii U. Drag figures onto the pad, move them between spaces and lift them off from a web page, without pausing the game to open the plugin menu.

This is a fork of [deReeperJosh/re_nsyshid](https://github.com/deReeperJosh/re_nsyshid), the Aroma plugin that emulates Toys-to-Life portals (Skylanders Portal of Power, Disney Infinity Base and LEGO Dimensions Toy Pad). Everything the original plugin does still works the same; this fork adds the web control page.

Requires the [Aroma](https://github.com/wiiu-env/Aroma) environment.

> [!IMPORTANT]
> The web control page currently only works with **LEGO Dimensions**. Skylanders and Disney Infinity figures still have to be loaded from the plugin menu.

> [!WARNING]
> Early release. The web page has been tested in browsers, but on-console testing is still in progress. Expect rough edges, and please report what you find.

## Features

- **Drag and drop** figures from your SD card onto any of the 7 Toy Pad spaces, with a mouse or a touchscreen
- **Move** figures between spaces — the game sees the figure lifted and put down, just like the in-game menu
- **Lift off** a figure by dragging it off the pad or tapping its ✕
- **Browse your folders** under `SD:/wiiu/re_nsyshid`, with a filter box for big collections
- **Favourites** row for the figures you swap most often
- **Live pad lights** — the page glows in the colours the game is lighting each pad
- Phone-first layout that also works side by side on a PC

## Install

1. Download `re_nsyshid-web-toypad-v0.1.zip` from [Releases](../../releases).
2. Extract it to the root of your SD card. It puts the plugin at `wiiu/environments/aroma/plugins/re_nsyshid.wps`, replacing any existing re_nsyshid.
3. Copy your LEGO Dimensions figure dumps (`.bin`) to `SD:/wiiu/re_nsyshid/`. Subfolders are fine.

## Use

1. On the Wii U, open the plugin menu with **L + Down + SELECT**.
2. Set **Emulation Status** to enabled and **Device to Emulate** to *Emulate Dimensions Toypad*. Do this before starting the game.
3. Open **Web Control Page**, turn on **Enable Web Control Page**, and note the address shown (for example `http://192.168.1.20:8853`).
4. Start LEGO Dimensions, then open that address in a browser on a phone or PC connected to the **same Wi-Fi** as the Wii U.

On a phone, press and hold a figure for a moment, then drag it. You can also tap a figure, then tap a space.

### If the page won't load

- Try `http://<address>:8853/ping` first — it should show `re_nsyshid`.
- The server only runs while a game or app is open, not on the Wii U boot screen.
- Check the **Server:** line in the plugin's Web Control Page menu:
  - *Listening, 0 requests* — the browser never reached the Wii U. Check the phone isn't on mobile data or a guest network.
  - *Stopped: …* — shows the error that stopped the server.

> [!NOTE]
> The page uses plain HTTP with no password. Only enable it on a network you trust.

## Original plugin features

The plugin menu still lets you load figures for all three devices:

- Choose a figure for each slot from **Skylander Manager**, **Infinity Manager** or **Dimensions Manager**.
- Remove a figure by highlighting its slot and pressing **X**.
- Toggle **Emulation Status** off to use a real USB portal instead.

It's best to choose the emulation status and device before starting a game; changing them mid-game can cause errors.

## Building

Building with Docker is recommended. Clone with submodules first:

```bash
git clone --recursive https://github.com/yaboimanguy-a11y/re_nsyshid.git
```

```bash
docker build . -t re_nsyshid_builder
```

```bash
docker run -it --rm -v ${PWD}:/project re_nsyshid_builder make
```

GitHub Actions also builds the plugin on every push; download it from the run's **Artifacts**.

## Credits

- [deReeperJosh](https://github.com/deReeperJosh) — re_nsyshid, the portal emulation and the original HTTP API this page builds on
- [GaryOderNichts](https://github.com/GaryOderNichts) — code from [re_nfpii](https://github.com/GaryOderNichts/re_nfpii)
- [tinyhttp](src/http/README.md) (Apache 2.0) and [MiniJson](https://github.com/zsmj2017/MiniJson) — HTTP server and JSON
- Web control page by Braden Haney with Claude

Licensed under GPLv2, like the original plugin.
