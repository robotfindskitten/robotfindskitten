nkidir = $(datadir)/games/robotfindskitten
nki_DATA = vanilla.nki

uninstall:
       rm -rf $(nkidir)
       if find "$(datadir)/games" -maxdepth 0 -empty | read ; then rmdir $(datadir)/games ; fi

