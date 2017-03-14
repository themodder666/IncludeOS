#!/bin/bash

# Script that builds all the files in the example folder
# Will print the output if the test fails

function getScriptAbsoluteDir {
    # @description used to get the script path
    # @param $1 the script $0 parameter
    local script_invoke_path="$1"
    local cwd=`pwd`

    # absolute path ? if so, the first character is a /
    if test "x${script_invoke_path:0:1}" = 'x/'
    then
        RESULT=`dirname "$script_invoke_path"`
    else
        RESULT=`dirname "$cwd/$script_invoke_path"`
    fi
}

script_invoke_path="$0"
script_name=`basename "$0"`
getScriptAbsoluteDir "$script_invoke_path"
script_absolute_dir=$RESULT

errors_present=0
echo -e ">>> Will now attempt to make all examples. Outpt from make will only be present if an error occured"

for dir in `ls -d $script_absolute_dir/../../../examples/*`
do
  BREAK=""
  cd $dir
  BASE=`basename $dir`
  echo -e "\n\n>>> Now making $BASE"
  mkdir -p build
  pushd build
  rm -rf *
  cmake .. > /tmp/build_test
  make >> /tmp/build_test || BREAK=1

  if [ "$BREAK" != "" ]
  then
     errors_present=$((errors_present+1))
     cat /tmp/build_test
  fi

done


# Clean up the files used
rm -f /tmp/build_test

exit $errors_present
