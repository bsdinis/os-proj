#!/usr/bin/zsh
#!/usr/bin/bash

make -C .. clean
make -C .. DEBUG=no
if [ $# -ne 1 ] 
then
  echo "usage: $0 <n_threads>";
  exit 1
fi

for file in ../inputs/*.txt
do
  echo $file | grep 512 
  if [[ $?  -ne 0 ]]
  then
    echo "============================================================"
    echo " == SOLVING FILE " $file
    echo "============================================================"
    echo
    ../bin/doTest.sh $1 ../inputs/$file
  fi
done

