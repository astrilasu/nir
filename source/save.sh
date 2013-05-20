mkdir mean;
for i in $(ls -tr *.fit)
  do
    command='ds9 '$i' -save mean/'$i' -exit';
    echo $command;
    $command;
  done
