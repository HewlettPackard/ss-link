#!/bin/bash

INFILE=$1
OUTFILE=$2

if [ $# -ne 2 ]; then
	echo "usage: $0 <input file> <output file>"
	exit
fi

if [ ! -f $INFILE ]; then
	echo "$0: input file \"${INFILE}\" does not exist"
	exit
fi

if [ -e $OUTFILE ]; then
	echo "$0: output file \"${OUTFILE}\" exists"
	exit
fi

count=0
date
while IFS= read -r LINE
do
	## filter lines
	if [[ -z "$LINE" ]] ; then
		continue;
	fi
	if [[ "$LINE" =~ "{" ]] ; then
		continue;
	fi
	if [[ "$LINE" =~ "}" ]] ; then
		continue;
	fi
	if [[ "$LINE" =~ ^.*define.* ]] ; then
		continue;
	fi
	##echo ">>$LINE<<"

	## clean
	CLEAN_COMMA=${LINE//","/""}
	CLEAN_HEX=${CLEAN_COMMA//"0x"/""}
	##echo ">>$CLEAN_HEX<<"

	## write
	LIST=($(echo "$CLEAN_HEX" | tr ' ' '\n'))
	for x in "${!LIST[@]}" ; do
		echo -ne "\\x${LIST[x]}" >> $OUTFILE
	done

	## forward progress indicator
	count=$((count + 1))
	if [[ $((count % 100)) -eq 0 ]] ; then
		echo -n "."
	fi
done < "$INFILE"
echo
date
