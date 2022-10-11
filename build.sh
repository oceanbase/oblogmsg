#!/bin/sh

TOPDIR=`readlink -f \`dirname $0\``
BUILD_SH=$TOPDIR/build.sh

ALL_ARGS=("$@")
DEP_DIR=${TOPDIR}/deps/3rd/usr/local/oceanbase/deps/devel
TOOLS_DIR=${TOPDIR}/deps/3rd/usr/local/oceanbase/devtools
CMAKE_COMMAND="${TOOLS_DIR}/bin/cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DBUILD_AS_MAIN_PROJECT=ON"

echo "$0 ${ALL_ARGS[@]}"

CPU_CORES=`grep -c ^processor /proc/cpuinfo`

NEED_INIT=false
NEED_MAKE=false
MAKE_ARGS=(-j $CPU_CORES)

# parse arguments
function parse_args
{
    for i in "${ALL_ARGS[@]}"; do
        if [[ "$i" == "--init" ]]
        then
            NEED_INIT=true
        elif [[ "$i" == "--make" ]]
        then
            NEED_MAKE=make
        elif [[ $NEED_MAKE == false ]]
        then
            BUILD_ARGS+=("$i")
        else
            MAKE_ARGS+=("$i")
        fi
    done
}

function usage
{
    echo -e "Usage:
\t./build.sh -h
\t./build.sh init
\t./build.sh clean
\t./build.sh [BuildType] [--init] [--make [MakeOptions]]

OPTIONS:
    BuildType => debug(default), release, errsim, dissearray, rpm
    MakeOptions => Options to make command, default: -j N

Examples:
\t# Build by debug mode and make with -j24.
\t./build.sh debug --make -j24
"
}

# try call init if --init given.
function try_init
{
    if [[ $NEED_INIT == true ]]
    then
        do_init || exit $?
    fi
}

# try call command make, if use give --make in command line.
function try_make
{
    if [[ $NEED_MAKE != false ]]
    then
        $NEED_MAKE "${MAKE_ARGS[@]}"
    fi
}

# dep_create.sh
function do_init
{
    (cd $TOPDIR/deps/3rd && bash dep_create.sh)
}

# create build directory and cd it.
function prepare_build_dir
{
    TYPE=$1
    mkdir -p $TOPDIR/build_$TYPE && cd $TOPDIR/build_$TYPE
}

# make build directory && cmake && make (if need)
function do_build
{
    TYPE=$1; shift
    prepare_build_dir $TYPE || return
    ${CMAKE_COMMAND} ${TOPDIR} "$@"
}

# clean build directories
function do_clean
{
    echo "cleaning..."
    find . -maxdepth 1 -type d -name 'build_*' | xargs rm -rf
}

function build
{
    set -- "${BUILD_ARGS[@]}"
    case "x$1" in
      xrelease)
        do_build "$@" -DCMAKE_BUILD_TYPE=RelWithDebInfo 
        ;;
      xdebug)
        do_build "$@" -DCMAKE_BUILD_TYPE=Debug
        ;;
      *)
        BUILD_ARGS=(debug "${BUILD_ARGS[@]}")
        build
        ;;
    esac
}

function main
{
    case "$1" in
        -h)
            usage
            ;;
        init)
            do_init
            ;;
        clean)
            do_clean
            ;;
        *)
            parse_args
            try_init
            build
            try_make
            ;;
    esac
}

main "$@"
