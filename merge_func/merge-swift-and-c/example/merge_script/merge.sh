#!/usr/bin/bash

LLVM_DIR=/proj/zyuxuanssf-PG0/zyuxuan/llvm-project-17/build/bin

function compile {
  swiftc -parse-as-library -emit-ir -o caller.ll ../caller/caller.swift
  $LLVM_DIR/clang++ -emit-llvm -S -o callee.ll ../callee/callee.cpp -std=c++17 
}

function merge {
  $LLVM_DIR/opt -passes=merge-swift-c -rename-callee-sc -S callee.ll -o callee_rename.ll
  $LLVM_DIR/llvm-link caller.ll callee_rename.ll -o caller_callee.ll
  $LLVM_DIR/opt -passes=merge-swift-c -merge-callee-sc -S caller_callee.ll -o merged.ll 
}

function link {
  llc -filetype=obj -relocation-model=pic -o caller.o caller.ll
  $LLVM_DIR/clang -fPIC -L/proj/zyuxuanssf-PG0/zyuxuan/swift-6.0.3/usr/lib/swift/linux caller.o -o caller -lswiftCore -lswiftSwiftOnoneSupport -lswift_Concurrency -lswift_StringProcessing -lswift_RegexParser -lswiftGlibc -lBlocksRuntime -ldispatch -lswiftDispatch -lFoundation -lFoundationEssentials -lFoundationInternationalization -lFoundationNetworking 
}

function build {
  compile
  merge
  link
}

function clean {
  rm caller
  rm *.ll
  rm *.o
}

case "$1" in
merge)
    build
    ;;
clean)
    clean
    ;;
esac
