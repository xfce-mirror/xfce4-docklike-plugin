# TODO

+ Make separate light and dark themes
+ Greyscale/pixelate minimized thumbnail images
+ Layering of XDG_DATA_DIRS is not working: files in /usr/share/applications, should not be used over those in home
+ Use X-Flatpak value for icon/desktop file names if it exists
+ Handle running out of dock space, overflow menu?
+ Make sure the label never overlaps the indicator
+ Use g_app_info_get_all() rather than finding apps ourselves
+ Use GAppInfoMonitor rather than inotify for better portability
+ Investigate GLib-CRITICAL from DnD, Timeouts
+ Move aliases to a text file so users can add stuff
+ Memory: something is not getting freed/unrefd when a group is closed
+ Test how well the updating thumbnails perform on a really slow system
+ Can we still crash the plugin?

## DONE

> + Dragged groups hover state gets stuck on
> + Docs: write the help page and update screenshots
> + Remove pinned application when desktop file is removed
> + Make automatic indicator orientation the default mode
> + Configurable thumbnail size, update sleep
> + Set indicator orientation based on panel orientation
> + Investigate GLib-CRITICAL from g_source_remove
> + Window count overlay
> + Improve the Dots indicator style
> + Option to set minimum width/height
> + New CSS indicator style, No indicator style
> + Make sure we are finding all the .desktop files
> + Associate new .desktop file in ~/.local/share/applications etc.
> + Editing launchers with exo-desktop-item-edit
> + Simplify Theme.cpp
> + Fix all build warnings
> + Indicator orientation
> + Basic orientation managing
> + Show only windows from active workspace
> + i18n
> + Settings UI
> + Basic workspaces managing
> + Drop down list of windows
> + Icon resizing
> + Pinable dock buttons
> + CSS class
> + Button reordering (d n'd)
> + XDG Desktop Files integration
