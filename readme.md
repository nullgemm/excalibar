### Excalibar
Excalibar is a fast and modular taskbar for X11, which provides good
plugin support and configurable aestethics. Its goal is to be the sword
amongst the taskbars: sharp and shiny. Because Excalibar is magical,
it is also lighter when unsheathed, and strives not to stress your cpu.

### Architecture 
Joke apart, Excalibar uses low-level library events, and is thus way
lighter than other taskbars (some of which loop through bash scripts).
It also gives a lot of freedom to plugins so they can be optimized.

### Configuration
Each section in the configuration file represents a plugin, and the core
program loads them by reading a section's name before copying the values
it holds. The order in which you write theses sections determines their
position in the bar. The "side" parameter can be set either to 0 or 1 to
indicate the plugin's alignment (left or right). Each global parameter
can be superseded for a particular plugin.

### Ingredients
Excalibar is written in C99 with xcb, cairo and pango as well as some
static routines from inih (which is linked to this repo as a submodule).
"And what's the recipe ? Excalibar weaponry !"

### Compiling and Installing
This repo contains submodules, to get them clone with:
```
git clone --recurse-submodules https://github.com/nullgemm/excalibar.git
```
First, compile and install libexcalibar:
```
cd lib
make
sudo make install
```
Then repeat these steps for the core program:
```
cd ../bar
make
sudo make install
```
While you are in the bar folder, edit and copy the config file:
```
nano excalibar.cfg
make cpconfig
```
Eventually, compile and install all the plugins: (no sudo here!)
```
cd ../plugins
make
make install
```
You can then launch excalibar as a daemon:
```
excalibar &
```
And close it properly with a sigterm:
```
pkill excalibar
```
The process survives after you quit your terminal ;)

### Greetings
Oxodao for the support, name approval and minecraft breaks

Raphaël for the memory I got the name idea from

Buckethead for the good music

Cairo and Pango contributors for the outstanding documentation
