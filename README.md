# re_nsyshid

re_nsyshid is a Wii U Plugin which allows end users to emulate Toys to Life USB peripheral devices such as the Skylander Portal of Power, the Disney Infinity Base and the LEGO Dimensions Toypad.

Requires the [Aroma](https://github.com/wiiu-env/Aroma) environment.  

> :warning: This plugin is still a work in progress, and some issues are expected!

The re_nsyshid plugin allows you to toggle the "Emulation Status" on and off - meaning that you can choose to either use your physical USB devices when the Emulation Status is disabled, or when enabled it will send commands to the device you have selected in the plugin config menu.

Includes some code repurposed from the [re_nfpii](https://github.com/GaryOderNichts/re_nfpii) plugin, big thank you to [GaryOderNichts](https://github.com/GaryOderNichts) for this!

> :warning: It is highly recommended to set your chosen Emulation Status and chosen Emulated Device prior to opening your game, as unexpected errors may occur when trying to toggle this in game.

## Usage

- Download the [latest release](https://github.com/deReeperJosh/re_nsyshid/releases) or the artifacts of latest nightly build from [here](https://nightly.link/deReeperJosh/re_nsyshid/workflows/build/main/re_nsyshid.zip).
- Copy the contents of the downloaded *`.zip`* file to your target environment.
- Copy your NFC Figure dumps to `SD:/wiiu/re_nsyshid`. Subfolders are also supported and can be browsed from the configuration menu.
- Open the plugin configuration menu with L + Down + SELECT.
- Enable Emulation, and choose an Emulated Device.
- Choose a Figure for your chosen device, for example enter Skylander Manager if you chose the Skylander Portal, and choose a Skylander to load in a specific slot.
- Figures can be quickly removed from slots by hovering over the currently loaded slot and pressing the X button

## Web Control Page (control the portal from your phone)

re_nsyshid can serve a small web page so you can place, move and remove LEGO Dimensions figures from a phone or PC while you play, without opening the plugin menu.

- Open the plugin menu (L + Down + SELECT), go to **Web Control Page** and turn on **Enable Web Control Page**.
- The same menu shows the address to open, e.g. `http://192.168.1.20:8853`. Your phone or PC must be on the same network as the Wii U.
- Drag a figure from your `SD:/wiiu/re_nsyshid` folders onto a space on the Toy Pad. On a phone, press and hold a figure for a moment, then drag. You can also tap a figure, then tap a space.
- Drag a figure between spaces to move it, drag it onto **Drop here to lift off the pad** to remove it, or tap its ✕.
- Star figures you swap often to keep them in a Favourites row at the top.

The page talks to the plugin over plain HTTP with no password, so only enable it on a network you trust.

## Planned features

In future releases, more USB devices are planned to be supported - such as the Kamen Rider Ride Gate

## Building

Building re_nsyshid using the Dockerfile is recommended:

```bash
# Build docker image (only needed once)
docker build . -t re_nsyshid_builder

# make 
docker run -it --rm -v ${PWD}:/project re_nsyshid_builder make

# make clean
docker run -it --rm -v ${PWD}:/project re_nsyshid_builder make clean
```
