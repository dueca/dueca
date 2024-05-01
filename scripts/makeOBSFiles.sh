#!/bin/bash
# Make a package tarball from svn

# usage message if no arguments
function usage()
{
    echo "Usage: `basename $0` [options] VERSION"
    echo "Options:"
    echo "   -h     use git master"
    echo "   -H ARG use alternative git branch"
    echo "   -s     use alternative .dsc file"
    echo "   -k     keep temporary dir"
    exit 1
}
#!/bin/sh

# defaults
SCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
TARGETDIR=`pwd`
NAME=dueca
PKGDIR=`mktemp -d /tmp/${NAME}.XXXXXXXX`
KEEPTMP=
OSCDIR="${HOME}/rpmbuild/home:repabuild/${NAME}"
OSCDIRV="${HOME}/rpmbuild/tu/home:repabuild/dueca-versioned"

# root of the repository with source
GITSERVER=git@github.com:dueca/dueca.git

# by default, pull from the official dueca remote
#GITREMOTE="--remote=${GITSERVER}"


# process input arguments
while getopts "khH:s:" optname
do
  case $optname in
      s) BASESPEC=`basename "$OPTARG" .dsc`;;
      h) GITBRANCHORTAG=master; GITREMOTE="" ;;
      H) GITBRANCHORTAG="$OPTARG"; GITREMOTE="" ;;
      k) KEEPTMP=1 ;;
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
if [ -z "$GITBRANCHORTAG" ]; then
    GITREMOTE="https://github.com/dueca/dueca/archive/v${VERSION}.tar.gz"
fi

# create a source rpm
function create_debfiles()
{
    echo "Using temporary dir $PKGDIR"
    mkdir ${PKGDIR}/dueca-${VERSION}
    if [ -z ${GITREMOTE} ]; then
        git archive --format=tar \
            ${GITBRANCHORTAG} | \
            tar -C $PKGDIR/dueca-${VERSION} -xf -
    else
        echo "curl -L ${GITREMOTE}"
        curl -L ${GITREMOTE} | tar -C $PKGDIR -xzf -
    fi
    if [ \! -d ${PKGDIR}/dueca-${VERSION}/dueca ]; then
        echo "Could not export copy"
        exit 1
    fi
    pushd $PKGDIR
    pushd dueca-${VERSION}

    if [ `pwd` != "$PKGDIR/dueca-${VERSION}" ]; then
        echo "Not operating in the right location"
        exit 1
    fi

    mkdir build
    pushd build

    # the configure should be minimal, and will be re-done in spec files
    cmake -G Ninja\
          -DBUILD_RTWPARSER=OFF \
          -DBUILD_EXTRA=OFF \
          -DBUILD_INTER=OFF \
          -DBUILD_HDF5=OFF \
          -DBUILD_DUSIME=OFF \
          -DBUILD_DMODULES=OFF \
          -DBUILD_DOC=OFF \
          -DBUILD_IP=OFF \
          -DBUILD_UDP=PFF \
          -DBUILD_WEBSOCK=OFF \
          -DBUILD_GTK2=OFF \
          -DBUILD_GTKMM=OFF \
          -DBUILD_GTK3=OFF \
          -DBUILD_GTKMM3=OFF \
          -DBUILD_GTKGLEXT=OFF \
          -DBUILD_GLUT=OFF \
          -DBUILD_GLUTGUI=OFF \
          -DBUILD_FLTK=OFF \
          -DBUILD_SHM=OFF \
          -DBUILD_X11GL=OFF \
          -DSVNHEAD=${SVNHEAD} \
          ..

    if [ \! -d obs ]; then
        echo "Could not configure"
        exit 1
    fi

    # to build/obs; dueca-versioned and dsc/spec files are here
    pushd obs

    # rpm-based distros
    cp $NAME.spec ../../..
    cp $NAME-versioned.spec ../../..
    cp $NAME-rpmlintrc ../..

    # first the versioned packages
    # default deb-based, currently focused on Ubuntu 20.04, 22.04, 24.04
    tar cvf ../../../debian-versioned.tar \
        --transform "s/debian-versioned/debian/" \
        debian-versioned
    cp dueca-versioned.dsc ../../../dueca-versioned.dsc
    cp $NAME.dsc ../../../$NAME.dsc

    # now hack/adapt for xUbuntu_18.04
    mv debian-versioned/control debian-versioned/control.bak
    sed -e 's/guile-2\.2-dev/guile-1\.8-dev/
            s/python3-xlwt/python-xlwt/' debian-versioned/control.bak > \
            debian-versioned/control
    tar cvf ../../../debian-versioned-xUbuntu_18.04.tar \
        --transform "s/debian-versioned/debian/" \
        debian-versioned
    sed -e 's/guile-2\.2-dev/guile-1\.8-dev/
            s/debian\.tar/debian-xUbuntu_18\.04.tar/
            s/python3-xlwt/python-xlwt/' dueca-versioned.dsc > \
                ../../../dueca-versioned-xUbuntu_18.04.dsc
    sed -e 's/guile-2\.2-dev/guile-1\.8-dev/
            s/debian\.tar/debian-xUbuntu_18\.04.tar/
            s/python3-xlwt/python-xlwt/' ${NAME}.dsc > \
                ../../../${NAME}-xUbuntu_18.04.dsc

    # Portfile for mac osx builds
    cp Portfile ../../..

    # back to main dueca folder, & clean the build
    popd
    popd
    rm -rf build

    # now to obs source folder
    pushd obs

    # default deb-based, currently focused on Ubuntu 20.04
    sed -e "s/@dueca_VERSION@/${VERSION}/" \
        debian/changelog.in >debian/changelog
    tar cvf ../../debian.tar debian

    # now hack/adapt for xUbuntu_18.04
    mv debian/control debian/control.bak
    sed -e 's/guile-2\.2-dev/guile-1\.8-dev/
            s/python3-xlwt/python-xlwt/' debian/control.bak > \
            debian/control
    tar cvf ../../debian-xUbuntu_18.04.tar \
        debian

    # back to the tmp folder with checked out source
    popd
    popd
    pwd

    # create source file here
    tar cfj dueca-${VERSION}.tar.bz2 dueca-${VERSION}

    rm -rf dueca-${VERSION}
    if [ -z "$KEEPTMP" -a -d "${OSCDIRV}" ]; then
        cp -f dueca*tar.bz2 ${OSCDIRV}
        mv -f dueca-versioned.dsc ${OSCDIRV}
        mv -f dueca-versioned-xUbuntu_18.04.dsc ${OSCDIRV}
        mv -f debian-versioned.tar ${OSCDIRV}/debian.tar
        mv -f debian-versioned-xUbuntu_18.04.tar \
           ${OSCDIRV}/debian-xUbuntu_18.04.tar
    fi

    FILES=`ls *`

    # OS X macports
    if [ -d $HOME/ports/science/dueca ]; then
        sha256=`openssl dgst -sha256 dueca-${VERSION}.tar.bz2 | cut -f2 -d' '`
        rmd160=`openssl dgst -ripemd160 dueca-${VERSION}.tar.bz2 | cut -f2 -d' '`
        sed -e "s/RMD160/${rmd160}/
                s/SHA256/${sha256}/" -i.bak Portfile
        cp -f Portfile $HOME/ports/science/dueca
        cp -f dueca-${VERSION}.tar.bz2 $HOME/ports/science/dueca/files
        echo "Created portfile"
    fi

    if [ -z "$KEEPTMP" -a -d "${OSCDIR}" ]; then
        mv -f $FILES "${OSCDIR}"
        echo "Copied"
        echo $FILES
        echo "to ${OSCDIR}"
        popd
        rm -rf $PKGDIR
    else
        popd
        echo "Created files $FILES"
        echo "You can find them in $PKGDIR"
    fi
}

create_debfiles
