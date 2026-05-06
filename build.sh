#!/usr/bin/env sh

exe_name=logs
build_dir=build
compiler=clang
comp_flags="-DLOGS_ENABLED=1
            -fno-builtin
            -fno-stack-protector
            -mbmi2
            -mlzcnt
            -O2
            -pedantic
            -std=c11
            -Wall
            -Werror
            -Wno-strict-aliasing"
link_flags="-flto
            -nostdlib
            -Qn
            -s
            -static
            -Wl,--build-id=none
            -Wl,-n"
sources="logs.c example.c"

for arg in "$@"; do declare $arg=1; done

project_dir=$(dirname $0)

if [ -v run ];
then
  if [ ! -e $project_dir/$build_dir/$exe_name ];
  then
    ./$0
  fi
  ./$project_dir/$build_dir/$exe_name
elif [ -v clean ];
then
  if [ -d $project_dir/$build_dir ];
  then
    rm -rf $project_dir/$build_dir && \
    echo Removed directory $project_dir/$build_dir
  fi
  if [ -e "Fluß_¼½¾_Öçé_ǅ.txt" ];
  then
    rm -rf "Fluß_¼½¾_Öçé_ǅ.txt" && \
    echo Removed file Fluß_¼½¾_Öçé_ǅ.txt
  fi
else
  pushd $project_dir >/dev/null
    mkdir -p $build_dir
    $compiler $comp_flags $link_flags $sources -o $build_dir/$exe_name && \
    echo Executable successfully created: $build_dir/$exe_name
  popd >/dev/null
fi