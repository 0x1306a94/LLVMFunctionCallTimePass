#!/bin/bash

set -e
set -o pipefail
set -u

VERSION=11.0.0
CUR_DIR=$PWD
DEPS_DIR=$PWD/deps
LLVM_DIR=$DEPS_DIR/llvm-$VERSION
CLANG_DIR=$DEPS_DIR/clang-$VERSION
mkdir -p $DEPS_DIR $LLVM_DIR $CLANG_DIR

cd $DEPS_DIR

wget https://github.com/llvm/llvm-project/archive/llvmorg-${VERSION}.zip -O ${LLVM_DIR}.zip
unzip ${LLVM_DIR}.zip -d $LLVM_DIR

wget https://github.com/llvm/llvm-project/releases/download/llvmorg-${VERSION}/clang+llvm-${VERSION}-x86_64-apple-darwin.tar.xz -O ${CLANG_DIR}.tar.xz
tar xf ${CLANG_DIR}.tar.xz -C $CLANG_DIR --strip-components 1

ln -sf $CUR_DIR/FunctionCallTime $LLVM_DIR/llvm-project-llvmorg-${VERSION}/llvm/lib/Transforms/FunctionCallTime

set +e

cat $LLVM_DIR/llvm-project-llvmorg-${VERSION}/llvm/lib/Transforms/LLVMBuild.txt | grep 'FunctionCallTime'
CODE=$?
echo $CODE
if [[ $CODE != 0 ]]; then
    sed 's/subdirectories.*/& FunctionCallTime/g' $LLVM_DIR/llvm-project-llvmorg-${VERSION}/llvm/lib/Transforms/LLVMBuild.txt > $LLVM_DIR/llvm-project-llvmorg-${VERSION}/llvm/lib/Transforms/LLVMBuild.txt.temp
    rm $LLVM_DIR/llvm-project-llvmorg-${VERSION}/llvm/lib/Transforms/LLVMBuild.txt
    mv $LLVM_DIR/llvm-project-llvmorg-${VERSION}/llvm/lib/Transforms/LLVMBuild.txt.temp $LLVM_DIR/llvm-project-llvmorg-${VERSION}/llvm/lib/Transforms/LLVMBuild.txt
fi

cat $LLVM_DIR/llvm-project-llvmorg-${VERSION}/llvm/lib/Transforms/CMakeLists.txt | grep 'FunctionCallTime'
CODE=$?
echo $CODE
if [[ $CODE != 0 ]]; then
    echo 'add_subdirectory(FunctionCallTime)' >> $LLVM_DIR/llvm-project-llvmorg-${VERSION}/llvm/lib/Transforms/CMakeLists.txt
fi
