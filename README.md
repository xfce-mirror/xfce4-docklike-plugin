# Docklike Taskbar for Xfce

A modern, minimalist taskbar for Xfce

Docklike Taskbar behaves similarly to many other desktop environments and operating systems. Wherein all application windows are grouped together as an icon and can be pinned to act as a launcher when the application is not running. Commonly referred to as a dock.

For usage instructions, keyboard shortcuts, and screenshots, see:
<https://docs.xfce.org/panel-plugins/xfce4-docklike-plugin/start>.

## Build & Install

```bash
tar xvf xfce4-docklike-plugin-0.4.0.tar.gz && cd xfce4-docklike-plugin-0.4.0
./configure
make
sudo make install
```

Use `./configure --prefix=/usr/local` to change install location

### From git

```bash
git clone https://gitlab.xfce.org/panel-plugins/xfce4-docklike-plugin.git && cd xfce4-docklike-plugin
./autogen.sh
make
sudo make install
```

Use `./autogen.sh --prefix=/usr/local` to change install location

## Reporting bugs

To assist with troubleshooting plugin issues, please run Xfce Panel in debugging mode, and include the relevant output in issues.

<https://gitlab.xfce.org/panel-plugins/xfce4-docklike-plugin/-/issues>

### Running Xfce Panel in debugging mode

- Open a terminal
- Quit the Xfce Panel using `xfce4-panel -q`
- Start the panel in debugging mode with `PANEL_DEBUG=1 G_MESSAGES_PREFIXED= G_MESSAGES_DEBUG=docklike xfce4-panel`
- Perform any actions you want to debug and copy the relevant output
- Stop debugging by pressing `Ctrl^C`
- Start the panel again using `xfce4-panel &`

The Xfce wiki has more details on panel debugging:
<https://docs.xfce.org/xfce/xfce4-panel/debugging>.
