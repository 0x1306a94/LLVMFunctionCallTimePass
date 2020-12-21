#!/usr/bin/env bash

set -e
set -o pipefail
set -u


VERSION=11.0.0
CUR_DIR=$PWD
DEPS_DIR=$PWD/deps
SOURCE_CODE_DIR=$DEPS_DIR/llvm-$VERSION/llvm-project-llvmorg-$VERSION
BUILD_DIR=$CUR_DIR/build_dir
LLVM_BUILD_DIR=$BUILD_DIR/build_llvm
LLVM_OUT_DIR=$BUILD_DIR/build_llvm_out

CLANG_BUILD_DIR=$BUILD_DIR/build_clang
CLANG_OUT_DIR=$BUILD_DIR/build_clang_out

XCODE_BUILD_DIR=$BUILD_DIR/build_xcode
XCODE_OUT_DIR=$BUILD_DIR/build_xcode_out

function build_llvm() {
    echo "generate llvm ninja build config ..."
    cd $LLVM_BUILD_DIR
    cmake -G "Ninja" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="10.14" \
    -DCMAKE_OSX_SYSROOT="macosx" \
    -DCMAKE_OSX_ARCHITECTURES='x86_64' \
    -DLLVM_TARGETS_TO_BUILD="X86" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=$LLVM_OUT_DIR \
    $SOURCE_CODE_DIR/llvm
    echo "ninja build llvm ..."
    ninja install
}

function build_clang() {
    echo "generate clang ninja build config ..."
    cd $CLANG_BUILD_DIR
    cmake -G "Ninja" \
    -DCMAKE_PREFIX_PATH=$LLVM_OUT_DIR/lib/cmake/llvm \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="10.14" \
    -DCMAKE_OSX_SYSROOT="macosx" \
    -DCMAKE_OSX_ARCHITECTURES='x86_64' \
    -DLLVM_TARGETS_TO_BUILD="X86" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=$CLANG_OUT_DIR \
    $SOURCE_CODE_DIR/clang
    echo "ninja build clang ..."
    ninja install
}

function generate_xcode_project() {
    cd $XCODE_BUILD_DIR
    cmake -G "Xcode" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="10.14" \
    -DCMAKE_OSX_SYSROOT="macosx" \
    -DCMAKE_OSX_ARCHITECTURES='x86_64' \
    -DLLVM_TARGETS_TO_BUILD="X86" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=$XCODE_OUT_DIR \
    $SOURCE_CODE_DIR/llvm
}


function clear_build() {
    echo "clear build dir"
    rm -rf $BUILD_DIR
}

function usage() {
    echo "Usage:"
    echo "build.sh -b -x"
    echo "Description:"
    echo "-b, build llvm and clang"
    echo "-x, generate xcode project"
    echo "-c, clear build dir"
    exit 0
}

function onCtrlC () {
    clear_build
    exit 0
}

BUILD_FLAG=0
GENERATE_XCODE_FLAG=0
CLEAR_BUILD_FLAG=0
while getopts 'hbxc' OPT; do
    case $OPT in
        b) BUILD_FLAG=1;;
        x) GENERATE_XCODE_FLAG=1;;
        c) CLEAR_BUILD_FLAG=1;;
        h) usage;;
        ?) usage;;
    esac
done


if [[ $CLEAR_BUILD_FLAG == 1 ]]; then
    clear_build
fi

if [[ $BUILD_FLAG == 1 ]]; then

    # trap 'onCtrlC' INT

    
    startTime_s=`date +%s`
    
    if [[ $GENERATE_XCODE_FLAG == 1 ]]; then
        mkdir -p $XCODE_BUILD_DIR
        generate_xcode_project
    else
        mkdir -p $LLVM_BUILD_DIR $CLANG_BUILD_DIR
        build_llvm
        build_clang
    fi 

    endTime_s=`date +%s`
    sumTime=$[ $endTime_s - $startTime_s ]

    echo "开始: $(date -r $startTime_s +%Y%m%d-%H:%M:%S)"
    echo "结束: $(date -r $endTime_s +%Y%m%d-%H:%M:%S)"
    hour=$[ $sumTime / 3600 ]
    minutes=$[ ($sumTime - ($hour * 3600)) / 60 ]
    seconds=$[ ($sumTime - ($hour * 3600)) % 60 ]
    echo "耗时：$hour:$minutes:$seconds"
fi




