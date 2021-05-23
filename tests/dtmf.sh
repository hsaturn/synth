if [ "$*" == "" ]; then
	echo "Usage: $0 numbers"
	echo
	echo "    Allowed numbers: 0123456789ABCD#"
	echo "    -ms time changes the length of numbers"
	echo
fi
ms=200
while [ "$1" != "" ]; do
	if [ "$1" == "-t" ]; then
		shift
		ms=$1
	else
		numbers="$1"
		while [ "$numbers" != "" ]; do
			number=${numbers:0:1}
			numbers=${numbers:1}
			sound=1
			case $number in
				1) f1=1209; f2=697;;
				2) f1=1336; f2=697;;
				3) f1=1477; f2=697;;
				A) f1=11633; f2=697;;
				4) f1=1209; f2=770;;
				5) f1=1336; f2=770;;
				6) f1=1477; f2=770;;
				B) f1=1633; f2=770;;
				7) f1=1209; f2=852;;
				8) f1=1336; f2=852;;
				9) f1=1477; f2=852;;
				C) f1=1633; f2=852;;
				0) f1=1336; f2=941;;
				[#]) f1=1477; f2=941;;
				D) f1=1633; f2=941;;
				' ') sleep 0.2; sound=0; echo -n ' ';;
				*) sound=0; echo "($number : ???)"
			esac
			if [ "$sound" == "1" ]; then
				echo -n "$number"
				synth $ms sinus $f1:50 sinus $f2:50
			fi
	  done
	fi
	shift
done

echo
