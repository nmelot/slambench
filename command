echo -en "\033c"; TYPE=RELEASE; rm kfusion/kfusion-*-{drake,cpp}; PATH=$HOME/.local/$(if [ "$TYPE" == "DEBUG" ]; then echo "debug"; else echo .; fi)/bin:$PATH make -C ~/git/drake-kfusion/ install CONFIG=$TYPE && (mkdir -p $TYPE) && (cd $TYPE && cmake .. -DCMAKE_BUILD_TYPE=$TYPE && make VERBOSE=1) && $TYPE/kfusion/kfusion-benchmark-drake -i ~/sandbox/slambench_datasets/living_room_traj2_loop.raw -s 4.8 -p 0.34,0.5,0.24 -z 4 -c 2 -r 1 -k 481.2,480,320,240

echo -en "\033c"; TYPE=RELEASE; rm kfusion/kfusion-*-{drake,cpp}; PATH=$HOME/.local/debug/bin:$PATH make -C ~/git/drake-kfusion/ install CONFIG=$TYPE && (mkdir -p $TYPE; exit $?) && (cd $TYPE && cmake .. -DCMAKE_BUILD_TYPE=$TYPE && make VERBOSE=1) && sleep 10 && LD_PRELOAD=~/.local/lib/librapl.so $TYPE/kfusion/kfusion-benchmark-drake -i ~/sandbox/slambench_datasets/living_room_traj2_loop_frame0-50.raw -s 4.8 -p 0.34,0.5,0.24 -z 4 -c 2 -r 1 -k 481.2,480,320,240 | tee drake-perf.log

cat drake-perf.log | cut -f 1,9 | tr '\t' ' ' | tr -s ' ' | tac | tail -n +3 | tac | tail +2 | sed 's/\s*$/ drake/g' | tr ' ' , > drake-stats.csv

## Repeat lines 3 and 5 for cpp

./drawplots.r

cat drake.perf | grep -E '(^[a-zA-Z0-9 ]*:\s[0-9]*\.[0-9]*$|^[0-9]*\s)' | while read i; do if [ "$(echo $i | grep "^[0-9]*\s")" != "" ]; then frame=$(echo $i | cut -f 1 -d ' '); else if [ "$frame" == "" ]; then frame=-1; fi; echo $(($frame + 1)),$(echo $i | sed 's/: /,/g'); fi; done > drake_detailed_perf.csv
