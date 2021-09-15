# Docklike Taskbar for Xfce

A modern, minimalist taskbar for Xfce

## Internationalization

- New translations: go to the `/po` directory, and create a new `[langcode].po` file by editing `xfce4-docklike-plugin.pot`.
- Updating translations: edit the `[langcode].po` file and translate new strings. Any lines containing `#, fuzzy` can be deleted if the translation they precede is correct.
- Open a pull request

#### Dependencies

+ libxfce4panel-2.0
+ libxfce4ui-2
+ gtk-3.0
+ cairo-1.16
+ libwnck-3.0
+ x11-1.6

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
