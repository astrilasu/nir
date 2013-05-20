for i in $(ls -tr mean/*.fit)
  do
    exposure=$(echo $i | awk -F'-' '{print $3}');
    echo $exposure            `imstat $i[590:690,462:562] | grep mean | awk '{print $4}'`
  done
