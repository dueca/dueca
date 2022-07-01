#!/bin/sh
# Make a package tarball from svn

# usage message if no arguments
function usage()
{
    echo "Usage: `basename $0` [options] VERSION"
    echo "Options:"
    echo "   -r RELEASE Release number for the rpm"
    echo "   -s SPEC    spec file name"
    exit 1
}
#!/bin/sh

# defaults
TARGETDIR=`pwd`
RPMREL=1
BASESPEC=dueca
PKGDIR=`mktemp -d`

# process input arguments
while getopts "r:s:" optname
do
  case $optname in
    r) RPMREL="$OPTARG" ;;
    s) BASESPEC=`basename "$OPTARG" .spec`;;
    [?]) usage
            exit 1;;
    esac
done
shift `expr $OPTIND - 1`
if test -z $1; then
  usage
  exit 1;
fi
VERSION=$1

# root of the repository with source
SVNROOT=$(svn info | sed -e '/^Repository Root:/!d; s/Repository Root: //')

# create a source rpm
function create_srpm()
{
    SVNTAG=`echo "D${VERSION}" | tr . -`
    echo "Using temporary dir $PKGDIR"
    ( cd $PKGDIR && \
      svn export ${SVNROOT}/tags/${SVNTAG} dueca-${VERSION} && \
      cd dueca-${VERSION}
      aclocal
      autoheader
      libtoolize --force --copy
      automake --add-missing --copy --gnu --warnings=no-portability
      autoconf
      # the configure should be minimal, and will be re-done in spec files
      ./configure --without-gtk --without-gtkmm --without-gtk2 \
          --without-gtkglext
      make ChangeLog SVNHEAD=tags/${SVNTAG}
      cd ..
      tar cfj dueca-${VERSION}.tar.bz2 dueca-${VERSION}
      cp dueca-${VERSION}/${BASESPEC}.spec ${BASESPEC}.spec.ini
      echo $RPMEXTRADEF >${BASESPEC}.spec
      sed -e "s/@rpm_release@/${RPMREL}/" ${BASESPEC}.spec.ini \
          >>${BASESPEC}.spec
      SRPMFILE=`rpmbuild --define "_sourcedir $PKGDIR" --define "_srcrpmdir $PKGDIR" -bs ${BASESPEC}.spec | sed -e '/^Wrote:/!d; s=Wrote: =='`
      mv ${SRPMFILE} $TARGETDIR
      echo "Created source rpm file $SRPMFILE" )
   rm -rf $PKGDIR
}

create_srpm

