## FILES="\"/usr/include/*.h\" \"/usr/local/include/*.h\""
FILES=""
BASEDIR=`pwd`
PROGDIR=. # `dirname $0`
GLOBAL_TAGS_FILE=$BASEDIR/system.tags
CFLAGS=""

# WxWindows libraries
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

# SDL libraries
SDL_PREFIX=`sdl-config --prefix 2>/dev/null`
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

# Pkg config libraries
PKG_CONFIG_INSTALLED=`pkg-config --version`
if [ ! -z "$PKG_CONFIG_INSTALLED" ]
then
## This is dangerously big database in RH 8.0, so only taking selective
## packages.
  ## PKG_CONFIG_GNOME_MODULES=`pkg-config --list-all 2>/dev/null | grep "gnome" | awk '{printf("%s ",  $1);}'`
  ## PKG_CONFIG_GTK_MODULES=`pkg-config --list-all 2>/dev/null | grep "gtk" | awk '{printf("%s ",  $1);}'`
  ## PKG_CONFIG_MODULES="$PKG_CONFIG_GNOME_MODULES $PKG_CONFIG_GTK_MODULES"
  PKG_CONFIG_MODULES=`pkg-config --list-all 2>/dev/null | awk '{printf("%s ",$1);}'`
  PKG_CONFIG_CFLAGS=`pkg-config --cflags $PKG_CONFIG_MODULES 2>/dev/null`
  for cflag in $PKG_CONFIG_CFLAGS
  do
    dir=`echo $cflag | sed 's/^-I//'`
    if [ -d "$dir" -a "$dir" != "/usr/include" -a "$dir" != "/usr/local/include" ]
    then
      FILES="$FILES \"$dir/*.h\" \"$dir/*/*.h\""
    fi
  done
  CFLAGS="$CFLAGS $PGK_CONFIG_CFLAGS"
fi
export CFLAGS
FILES_COUNT=`echo $FILES | wc -w`
echo "[Number of files to be scanned: $FILES_COUNT]"
echo "[...........................................................................]"
echo "[. Generating System tags. This may take several minutes (1 min to 20 mins}.]"
echo "[. You can go out, have a coffee and return back in 20 mins.]...............]"
echo "[...........................................................................]"
$PROGDIR/tm_global_tags $GLOBAL_TAGS_FILE $FILES 2>/dev/null
