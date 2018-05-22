Overview of path-returning methods
==================================

This section lists Pathie’s methods for retrieval of system or user
standard paths along with possible examples. It might serve as an
overview if you don’t want to read the documentation of all those
methods.

The _Standard_ column denotes the standard the specified path was taken from under UNIX; Windows
has no applicable standards it follows for its directory structure. The _Type_ column
specified whether this path varies on a per-user basis (_User_) or is a fixed system
path that is the same for all users (_System_); _Other_ means the path is unrelated
to User information.

On Windows, several paths may share the same location. This is because Windows
doesn’t have such a detailed filesystem layout structure as UNIX systems have.
If your application needs to run on Windows also, you should be careful to avoid
file naming conflicts in those directories.

The global paths given below for UNIX are those you’ll receive if you called Path::set_global_dir_default()
with `LOCALPATH_NORMAL` as its argument. You can also use `LOCALPATH_LOCAL` to instruct
the `global_*_dir()` methods to return the corresponding paths under the system-local
hierarchy (mostly `/usr/local` and `/var/local`) instead.

Method|Standard|Type|Typical UNIX return value|Typical Windows return value
------|--------|----|-------------------------|----------------------------
appentries_dir()|[XDG]|User|/home/user/.local/share/applications|C:/Users/user/Start Menu
cache_dir()|[XDG]|User|/home/user/.cache|C:/Users/user/AppData/Local
config_dir()|[XDG]|User|/home/user/.config|C:/Users/user/AppData/Roaming
data_dir()|[XDG]|User|/home/user/.local/share|C:/Users/user/AppData/Roaming
desktop_dir()|[XDGU]|User|/home/user/Desktop|C:/Users/user/Desktop
documents_dir()|[XDGU]|User|/home/user/Documents|C:/Users/user/Documents
download_dir()|[XDGU]|User|/home/user/Downloads|N/A
exe()|N/A|Other|/home/user/Downloads/software/bin/foo|C:/Users/user/Downloads/software/foo.exe
global_appentries_dir()|[FHS]|System|/usr/share/applications|C:/Users/All Users/Start Menu
global_cache_dir()|[FHS]|System|/var/cache|C:/Users/All Users/Application Data
global_config_dir()|[FHS]|System|/etc|C:/Windows/System32
global_immutable_data_dir()|[FHS]|System|/usr/share|C:/Windows/System32
global_mutable_data_dir()|[FHS]|System|/var/lib|C:/Users/All Users/Application Data
global_programs_dir()|[FHS]|System|/opt|C:/Program Files
global_runtime_dir()|[FHS]|System|/run _or_ /var/run|C:/Temp
home()|[FHS]|User|/home/user|C:/Users/user
music_dir()|[XDGU]|User|/home/user/Music|C:/Users/user/Music
pictures_dir()|[XDGU]|User|/home/user/Pictures|C:/Users/user/Pictures
publicshare_dir()|[XDGU]|User|/home/user/Public|C:/Users/user/AppData/Roaming/Microsoft/Windows/Network Shortcuts
pwd()|[POSIX]|Other|/home/user/Downloads/software|C:/Users/user/Downloads/software
runtime_dir()|[XDG]|User|/run/user/1000|C:/Users/user/AppData/Local/Temp
temp_dir()|[FHS]|System|/tmp|C:/Users/user/AppData/Local/Temp
templates_dir()|[XDGU]|User|/home/user/Templates|C:/Users/user/AppData/Roaming/Microsoft/Windows/Templates
videos_dir()|[XDGU]|User|/home/user/Videos|C:/Users/user/Videos

[FHS]: http://refspecs.linuxfoundation.org/FHS_2.3/
[XDG]: http://standards.freedesktop.org/basedir-spec/basedir-spec-latest.html
[XDGU]: http://freedesktop.org/wiki/Software/xdg-user-dirs/
[POSIX]: http://pubs.opengroup.org/onlinepubs/9699919799/
