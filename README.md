This is the source code for NASU Standalone 1.1.1.

It's not exactly easy to understand, I don't leave any comments or anything in my code.

Support for Windows has been temporarily dropped until I find a way to get SFML 2.0 to work there. Unluckily, there's no SFML 1.6 version of the game to include the changes in this version, just like there was a 1.0.3 for 1.1.

On Linux: Use the makefile. Just make and then sudo make install. I also provided targets "uninstall" and "reinstall". Obviously, it requires gcc and sfml.

As of today, I still have to rewrite everything from scratch AGAIN because SFML got yet another huge fucking overhaul in a minor version increase.

 - Marisa Kirisame - Sep 5th, 2012
