#!/bin/bash

LOG_DESTINATION=/var/gdclog

if [ -e gdc.lock ] ; then
	echo -e "GDC already running.\n"
	exit 1
fi

touch gdc.lock

games=(
	"CStrike"
)

email_test=(
	"self@example.com"
)

email_all=(
	"self@example.com"
)

email_cstrike=(
	"css-op@example.com"
	"css-op2@example.com"
	"css-op3@example.com"
)

for i in "${games[@]}"
do
	lcgame=$(echo $i | tr '[A-Z]' '[a-z]')

	filename=$(date +%Y-%m-%d-%H%M%S)-${lcgame}.log
	fullfilename=log/$filename

	./gdc_${lcgame}.sh auto 2>&1 | tee -a ${fullfilename}

	if [ ${PIPESTATUS[0]} -ne 0 ] ; then
	       	echo -e "No change. Removing useless log.\n"
	        rm $fullfilename
	else
		cp $fullfilename $LOG_DESTINATION


		if [ "$1" == "test" ] ; then
			for j in "${email_test[@]}"
			do
				echo -e "Sending test email to ${j}\n"
				mutt -s "$i GDC update" $j < $fullfilename
			done

			if [ "$2" == "log" ] ; then
				# when testing, don't write to log unless we really want the bot to show it
				echo $(date +%s),${filename},$i >> ~/gdclog
			fi
		else
			# log for pickup by bot
			echo $(date +%s),${filename},$i >> ~/gdclog

			for j in "${email_all[@]}"
			do
				echo -e "Sending email to ${j}\n"
			        mutt -s "$i GDC update" $j < $fullfilename
			done

			varname=email_$lcgame
			eval k=\${#$varname[@]}

			max=($k - 1)

			for (( m=0; m < $max; m++ ));
			do
				eval k=\${$varname[m]}
				echo -e "Sending email to ${k}\n"
				mutt -s "$i GDC update" $k < $fullfilename
			done
		fi  # just testing

	fi
done

rm -f gdc.lock
