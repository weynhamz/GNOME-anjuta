FILES="/usr/include/*.h /usr/local/include/*.h"
BASEDIR=`pwd`
PROGDIR=`dirname $0`
GLOBAL_TAGS_FILE=$BASEDIR/system.tags
CFLAGS=""

GNOME_PREFIX=`gnome-config --prefix gnome 2>/dev/null`
if [ ! -z "$GNOME_PREFIX" ]
then
  GNOME_MODULES="gnome gnomeui glib gtk gnorba idl"
  for file in $GNOME_PREFIX/lib/*Conf.sh
  do
    module=`echo $file | sed 's/^.*\///' | sed 's/Conf\.sh//'`
    GNOME_MODULES="$GNOME_MODULES $module"
  done
  PKG_CONFIG_MODULES=`pkg-config --list-all | awk '{printf("%s ",  $1);}'`
  GNOME_CFLAGS=`gnome-config --cflags $GNOME_MODULES`
  PKG_CONFIG_CFLAGS=`pkg-config --cflags $PKG_CONFIG_MODULES`
  for cflag in $GNOME_CFLAGS $PKG_CONFIG_CFLAGS
  do
    dir=`echo $cflag | sed 's/^-I//'`
    if [ -d "$dir" -a "$dir" != "/usr/include" -a "$dir" != "/usr/local/include" ]
    then
      FILES="$FILES $dir/*.h $dir/*/*.h"
    fi
  done
  CFLAGS="$GNOME_CFLAGS $PGK_CONFIG_CFLAGS"
#  echo "Files are $FILES"
#  echo "CFLAGS are $CFLAGS"
fi
export CFLAGS
$PROGDIR/tm_global_tags $GLOBAL_TAGS_FILE "$FILES" 2>/dev/null
