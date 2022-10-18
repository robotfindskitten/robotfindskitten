# robotfindskitten

In 1997, Peter A. Peterson II held a contest for his now-defunct Nerth Pork magazine.
The goal: come up with the best piece of art entitled "robotfindskitten".
Leonard Richardson submitted the winning (read: only) entry, a [Zen Simulation](https://stackoverflow.com/questions/10140069/what-in-the-world-is-a-zen-simulator/15147633#15147633).

```
┌──────────────────────────────────────────────────────────────────────┐
│                                                                      │
│       c                         a                                    │
│               yn                      [                              │
│                                                                     _│
│                                  #                                   │
│                            a                           p             │
│                                                                      │
│                               .                       Y              │
│ u                                                      N             │
│|                                   i                                 │
│                           M                       H               g  │
│        )                      P                                      │
│                                                                      │
│                                      w                           8   │
└──────────────────────────────────────────────────────────────────────┘
```

## Obtaining

robotfindskitten is available from most Unix distributions.

  * `sudo apt install robotfindskitten`  # [Debian](https://packages.debian.org/stable/robotfindskitten), Ubuntu, etc
  * `sudo dnf install robotfindskitten`  # Fedora, etc
  * `sudo pkg install robotfindskitten`  # [FreeBSD](https://www.freshports.org/games/robotfindskitten)
  * `brew install robotfindskitten`  # [macOS Homebrew](https://formulae.brew.sh/formula/robotfindskitten)
  * `guix install robotfindskitten`  # GNU Guix

As of this writing, [Finnix](https://www.finnix.org/) is the only distribution to carry robotfindskitten pre-installed.

If your distribution does not have robotfindskitten, contact them and politely ask them to distribute it.
It's in their best interest.

## Building from source

To build robotfindskitten from source, you will need GNU and TeX tools.
On Debian-based systems, the following build dependencies will be sufficient:

```
sudo apt install build-essential gnulib libncurses-dev texinfo texlive-latex-recommended
```

Generate/update autoconf/automake scripts.
This is required when building from git source, but is recommended for releases too.

```
autoreconf -ifv
```

Configure, make, install.

```
./configure
make
sudo make install
```
