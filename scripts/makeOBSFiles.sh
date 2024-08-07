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
    popd

    # default deb-based, currently focused on Ubuntu 24.04
    pushd obs
    sed -e "s/@dueca_VERSION@/${VERSION}/" \
        debian/changelog.in >debian/changelog
    tar cvf ../../debian.tar debian

    # control will later be modified for different build versions
    mv debian/control debian/control.bak
    popd

    # to build/obs; dueca-versioned and dsc/spec files are here
    pushd build/obs

    # rpm-based distros
    cp $NAME.spec ../../..
    cp $NAME-versioned.spec ../../..
    cp $NAME-rpmlintrc ../..

    # build description for the default config
    cp $NAME.dsc ../../../$NAME.dsc

    # the versioned packages
    # default deb-based, currently focused on Ubuntu 24.04
    tar cvf ../../../debian-versioned.tar \
        --transform "s/debian-versioned/debian/" \
        debian-versioned
    cp dueca-versioned.dsc ../../../dueca-versioned.dsc

    # variants in guile use, 2.0 for 22.04, 20.04, 1.8 for 18.04
    # also this control will later be modified for different build versions
    mv debian-versioned/control debian-versioned/control.bak
    popd

    # now hack/adapt for xUbuntu_18.04
    # base version
    pushd obs
    sed -e 's/guile-2\.2-dev/guile-1\.8-dev/
            s/python3-xlwt/python-xlwt/' debian/control.bak > \
            debian/control
    tar cvf ../../debian-xUbuntu_18.04.tar \
        debian
    popd

    # versioned
    pushd build/obs
    sed -e 's/guile-2\.2-dev/guile-1\.8-dev/
            s/python3-xlwt/python-xlwt/' debian-versioned/control.bak > \
            debian-versioned/control
    tar cvf ../../../debian-versioned-xUbuntu_18.04.tar \
        --transform "s/debian-versioned/debian/" \
        debian-versioned

    # and the dsc files, versioned and normal
    sed -e 's/guile-2\.2-dev/guile-1\.8-dev/
            s/debian\.tar/debian-xUbuntu_18\.04.tar/
            s/python3-xlwt/python-xlwt/' dueca-versioned.dsc > \
                ../../../dueca-versioned-xUbuntu_18.04.dsc
    sed -e 's/guile-2\.2-dev/guile-1\.8-dev/
            s/debian\.tar/debian-xUbuntu_18\.04.tar/
            s/python3-xlwt/python-xlwt/' ${NAME}.dsc > \
                ../../../${NAME}-xUbuntu_18.04.dsc
    popd

    # and for xUbuntu 20.04
    for VER in 20.04; do

        # base version debian folder
        pushd obs
        sed -e 's/guile-2\.2-dev/guile-2\.0-dev/' debian/control.bak > \
            debian/control
        tar cvf ../../debian-xUbuntu_${VER}.tar \
            debian
        popd

        # versioned version debian folder and the dsc files
        pushd build/obs
        sed -e 's/guile-2\.2-dev/guile-2\.0-dev/' \
            debian-versioned/control.bak > \
            debian-versioned/control
        tar cvf ../../../debian-versioned-xUbuntu_${VER}.tar \
            --transform "s/debian-versioned/debian/" \
            debian-versioned
        sed -e "s/guile-2\.2-dev/guile-2\.0-dev/
            s/debian\.tar/debian-xUbuntu_${VER}.tar/" dueca.dsc > \
                ../../../dueca-xUbuntu_${VER}.dsc
        sed -e "s/guile-2\.2-dev/guile-2\.0-dev/
            s/debian\.tar/debian-xUbuntu_${VER}.tar/" dueca-versioned.dsc > \
                ../../../dueca-versioned-xUbuntu_${VER}.dsc
        popd
    done

    # Portfile for mac osx builds
    pushd build/obs
    cp Portfile ../../..

    # back to main dueca folder, & clean the build
    popd

    if test -d build; then
        rm -rf build
    else
        echo "Cannot find build dir, at `pwd`"
        exit 1
    fi

    # back to the temporary
    popd

    # create source file here
    tar cfj dueca-${VERSION}.tar.bz2 dueca-${VERSION}

    if test -d dueca-${VERSION}; then
        rm -rf dueca-${VERSION}
    else
        echo "Cannot find checked out dueca copy, at `pwd`"
        exit 1
    fi

    if [ -z "$KEEPTMP" -a -d "${OSCDIRV}" ]; then
        cp -f dueca*tar.bz2 ${OSCDIRV}
        mv -f debian-versioned.tar ${OSCDIRV}/debian.tar
        mv -f dueca-versioned.dsc ${OSCDIRV}
        mv -f dueca-versioned.spec ${OSCDIRV}
        for VER in 18.04 20.04; do
            mv -f dueca-versioned-xUbuntu_${VER}.dsc ${OSCDIRV}
            mv -f debian-versioned-xUbuntu_${VER}.tar \
               ${OSCDIRV}/debian-xUbuntu_${VER}.tar
        done
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
        cp -f dueca*tar.bz2 ${OSCDIR}
        mv -f debian.tar ${OSCDIR}/debian.tar
        mv -f dueca.dsc ${OSCDIR}
        mv -f dueca.spec ${OSCDIR}
        for VER in 18.04 20.04; do
            mv -f dueca-xUbuntu_${VER}.dsc ${OSCDIR}
            mv -f debian-xUbuntu_${VER}.tar \
               ${OSCDIR}/debian-xUbuntu_${VER}.tar
        done
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
