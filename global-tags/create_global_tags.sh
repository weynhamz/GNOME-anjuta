FILES="\"/usr/include/*.h\" \"/usr/local/include/*.h\""
BASEDIR=`pwd`
PROGDIR=. # `dirname $0`
GLOBAL_TAGS_FILE=$BASEDIR/system.tags
CFLAGS=""

WX_PREFIX=`wx-config --prefix 2>/dev/null`
if [ ! -z "$WX_PREFIX" ]; then
  WX_CFLAGS=`wx-config --cxxflags`
  for cflag in $WX_CFLAGS
  do
    dir=`echo $cflag | sed 's/^-I//'`
    if [ -d "$dir" -a ! -e "$dir/wx/setup.h" ]
    then
      FILES="$FILES \"$dir/wx/*.h\" \"$dir/wx/*/*.h\""
    fi
  done
  CFLAGS="$CFLAGS $WX_CFLAGS"
fi

SDL_PREFIX=`sdl-config --prefix`
if [ ! -z "$SDL_PREFIX" ]; then
  SDL_CFLAGS=`sdl-config --cflags`
  for cflag in $SDL_CFLAGS
  do
    dir=`echo $cflag | sed 's/^-I//'`
    if [ -d "$dir" ]
    then
      FILES="$FILES \"$dir/*.h\""
    fi
  done
  CFLAGS="$CFLAGS $SDL_CFLAGS"
fi

GNOME_PREFIX=`gnome-config --prefix gnome 2>/dev/null`
if [ ! -z "$GNOME_PREFIX" ]
then
  GNOME_MODULES="applets bonobo_conf bonobo bonobox bonobox_print capplet \
    config_archiver docklets eel gal gdk_pixbuf gdk_pixbuf_xlib gdl \
    gdome ghttp gnome_build gnomecanvaspixbuf gnomemm gnumeric gpilot \
    gtkhtml libart libglade libgtop libguppi libIDL libole2 librsvg \
    nautilus oaf obGnome pong_bonobo pong pong_glade pong_manual \
    print vfs xml2 xml xslt"
  for file in $GNOME_PREFIX/lib/*Conf.sh
  do
    module=`echo $file | sed 's/^.*\///' | sed 's/Conf\.sh//'`
    GNOME_MODULES="$GNOME_MODULES $module"
  done
  PKG_CONFIG_MODULES=`pkg-config --list-all 2>/dev/null | awk '{printf("%s ",  $1);}'`
  GNOME_CFLAGS=`gnome-config --cflags $GNOME_MODULES 2>/dev/null`
  PKG_CONFIG_CFLAGS=`pkg-config --cflags $PKG_CONFIG_MODULES 2>/dev/null`
  for cflag in $GNOME_CFLAGS $PKG_CONFIG_CFLAGS
  do
    dir=`echo $cflag | sed 's/^-I//'`
    if [ -d "$dir" -a "$dir" != "/usr/include" -a "$dir" != "/usr/local/include" ]
    then
      FILES="$FILES \"$dir/*.h\" \"$dir/*/*.h\""
    fi
  done
  CFLAGS="$CFLAGS $GNOME_CFLAGS $PGK_CONFIG_CFLAGS"
fi
export CFLAGS
$PROGDIR/tm_global_tags $GLOBAL_TAGS_FILE $FILES 2>/dev/null
