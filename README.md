# Docklike Taskbar for Xfce

A modern, minimalist taskbar for Xfce

![screenshot](https://i.imgur.com/b4qDHCB.jpg)

## Internationalization

- New translations: go to the `/po` directory, and create a new `[langcode].po` file by editing `xfce4-docklike-plugin.pot`.
- Updating translations: edit the `[langcode].po` file and translate new strings. Any lines containing `#, fuzzy` can be deleted if the translation they precede is correct.
- Open a pull request

## Build & Install

```bash
tar xvf xfce4-docklike-plugin-0.3.0.tar.gz && cd xfce4-docklike-plugin-0.3.0
./configure
make
sudo make install
```

Use `./configure --prefix=/usr/local` to change install location

### From git

```bash
git clone https://github.com/davekeogh/xfce4-docklike-plugin && cd xfce4-docklike-plugin
./autogen.sh
make
sudo make install
```

Use `./autogen.sh --prefix=/usr/local` to change install location

### 3rd party packages

Provided by users. Submit a pull request if you have one for your distribution.

#### ArchLinux User Repository (AUR)

`yay -S xfce4-docklike-plugin-git`

#### Xubuntu 20.04 and above

```bash
sudo add-apt-repository ppa:xubuntu-dev/extras
sudo apt update
sudo apt install xfce4-docklike-plugin
```

#### Dependencies

+ libxfce4panel-2.0
+ libxfce4ui-2
+ gtk-3.0
+ cairo-1.16
+ libwnck-3.0
+ x11-1.6

To get all the dependencies you need on Ubuntu/Debian-like distros:

```bash
sudo apt install build-essential git xfce4-dev-tools libxfce4ui-2-dev libxfce4panel-2.0-dev libgtk-3-dev libcairo2-dev libwnck-3-dev libx11-dev
```

## Features & Screenshots

Pinnable applications, grouped windows, and desktop actions:

![screenshot](https://imgur.com/oTGuqxC.png)

Use Ctrl to reorder your applications or access the settings panel (with right click):

![screenshot](https://i.imgur.com/CUFp6QP.jpg)

Fast application switching : by hovering mouse while button pressed or scrolling:

![screenshot](https://i.imgur.com/bpR1E6j.jpg)

Window previews (disabled by default):

![screenshot](https://imgur.com/GahMRuw.jpg)

Use your Super key to start or switch applications extremely fast:

![screenshot](https://i.imgur.com/W69mmD2.png)

Different indicator styles:

![screenshot](https://i.imgur.com/9nWqkCG.jpg)
