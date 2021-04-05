while [ "$1" != "" ]; do
	if [ "$1" == "1" ]; then
		f1=1209
		f2=697
	fi
	if [ "$1" == "2" ]; then
		f1=1336
		f2=697
	fi
	if [ "$1" == "3" ]; then
		f1=1477
		f2=697
	fi
	if [ "$1" == "A" ]; then
		f1=11633
		f2=697
	fi
	if [ "$1" == "4" ]; then
		f1=1209
		f2=770
	fi
	if [ "$1" == "5" ]; then
		f1=1336
		f2=770
	fi
	if [ "$1" == "6" ]; then
		f1=1477
		f2=770
	fi
	if [ "$1" == "B" ]; then
		f1=1633
		f2=770
	fi
	if [ "$1" == "7" ]; then
		f1=1209
		f2=852
	fi
	if [ "$1" == "8" ]; then
		f1=1336
		f2=852
	fi
	if [ "$1" == "9" ]; then
		f1=1477
		f2=852
	fi
	if [ "$1" == "C" ]; then
		f1=1633
		f2=852
	fi
	if [ "$1" == "0" ]; then
		f1=1336
		f2=941
	fi
	if [ "$1" == "#" ]; then
		f1=1477
		f2=941
	fi
	if [ "$1" == "D" ]; then
		f1=1633
		f2=941
	fi
	synth 200 sinus $f1:50 sinus $f2:50
	shift
done
