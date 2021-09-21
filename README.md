# Docklike Taskbar for Xfce

A modern, minimalist taskbar for Xfce

## Documentation & Screenshots

For usage instructions, keyboard shortcuts, and screenshots, see <https://docs.xfce.org/panel-plugins/xfce4-docklike-plugin/start>.

### Dependencies

- libxfce4panel-2.0
- libxfce4ui-2
- gtk-3.0
- cairo-1.16
- libwnck-3.0
- x11-1.6
- exo-utils (for exo-desktop-item-edit)

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
git clone https://gitlab.xfce.org/panel-plugins/xfce4-docklike-plugin.git && cd xfce4-docklike-plugin
./autogen.sh
make
sudo make install
```

Use `./autogen.sh --prefix=/usr/local` to change install location

## Reporting bugs

<https://gitlab.xfce.org/panel-plugins/xfce4-docklike-plugin/-/issues>
