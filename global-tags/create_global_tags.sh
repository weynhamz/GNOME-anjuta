## FILES="\"/usr/include/*.h\" \"/usr/local/include/*.h\""
FILES=""
BASEDIR=`pwd`
PROGDIR=. # `dirname $0`
GLOBAL_TAGS_FILE=$BASEDIR/system.tags
CFLAGS=""

# WxWindows libraries
WX_CONFIG=`which wx-config 2>/dev/null`
if ( [ ! -z $WX_CONFIG ] && [ -x $WX_CONFIG ] ) ; then
  echo "Found wxWindows libraries ... $WX_CONFIG"
  WX_CFLAGS=`$WX_CONFIG --cxxflags`
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
SDL_CONFIG=`which sdl-config 2>/dev/null`
if ( [ ! -z $SDL_CONFIG ] && [ -x $SDL_CONFIG ] ) ; then
  echo "Found SDL libraries ... $SDL_CONFIG"
  SDL_CFLAGS=`$SDL_CONFIG --cflags`
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
PKG_CONFIG=`which pkg-config 2>/dev/null`
if ( [ ! -z $PKG_CONFIG ] && [ -x $PKG_CONFIG ] ) ; then
  echo "Found pkg-config ... $PKG_CONFIG"
  PKG_CONFIG_MODULES=`$PKG_CONFIG --list-all 2>/dev/null | awk '{printf("%s ",$1);}'`
  PKG_CONFIG_CFLAGS=`$PKG_CONFIG --cflags $PKG_CONFIG_MODULES 2>/dev/null`
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
