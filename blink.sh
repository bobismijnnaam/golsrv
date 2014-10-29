gpio mode $1 output
while true
do
	gpio write $1 0
	sleep 1
	gpio write $1 1
	sleep 1
done
