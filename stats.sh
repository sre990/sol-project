#!/bin/bash

if [ $# -eq 0 ]; then
	echo "Fatal error: please specify a log file path"
	exit 1
fi

BOLD="\033[4m"
RESET="\033[0m"
LOG_FILE=$1


echo -e "-${BOLD}READING OPERATIONS${RESET}-"
# count lines where the regex appears
READFILE=$(grep "readFile" -c $LOG_FILE)
READNFILES=$(grep "readNFiles" -c $LOG_FILE)
# bytes are read after the arrow ->
READFILE_BYTES=$(grep -E "readFile.*->" $LOG_FILE | grep -oE '[^ ]+$' | sed -e 's/\.//g' | { sum=0; while read num; do ((sum+=num)); done; echo $sum; })
READNFILES_BYTES=$(grep -E "readNFiles.*->" $LOG_FILE | grep -oE '[^ ]+$' | sed -e 's/\.//g' | { sum=0; while read num; do ((sum+=num)); done; echo $sum; })
READS=$((READFILE+READNFILES))
READ_BYTES=$((READFILE_BYTES+READNFILES_BYTES))

echo -e "readFile operations: ${READFILE}."
echo -e "readFile: ${READFILE_BYTES} bytes."
echo -e "readNFiles operations: ${READNFILES}."
echo -e "readNFiles: ${READNFILES_BYTES} bytes."
echo -e "total number of reads: ${READS}."
echo -e "total reading operations size: ${READ_BYTES} bytes."

# making sure we are not dividing by 0
if [ ${READFILE} -gt 0 ]; then
	MEAN_READFILES=$(echo "${READFILE_BYTES} / ${READFILE}" | bc -l)
	echo -e "readFile mean: ${MEAN_READFILES} bytes."
fi
if [ ${READNFILES} -gt 0 ]; then
	MEAN_READNFILES=$(echo "${READNFILES_BYTES} / ${READNFILES}" | bc -l)
	echo -e "readNFiles mean: ${MEAN_READNFILES} bytes."
fi
if [ ${READS} -gt 0 ]; then
	MEAN_READ=$(echo "${READ_BYTES} / ${READS}" | bc -l)
	echo -e "total mean: ${MEAN_READ} bytes."
fi


echo -e "-${BOLD}WRITING OPERATIONS${RESET}-"
# count lines where the regex appears
WRITEFILE=$(grep "writeFile" -c $LOG_FILE)
APPENDTOFILE=$(grep "appendToFile" -c $LOG_FILE)
WRITES=$((WRITEFILE+APPENDTOFILE))
# bytes are read after the arrow ->
WRITEFILE_BYTES=$(grep -E "writeFile.*->" $LOG_FILE | grep -oE '[^ ]+$' | sed -e 's/\.//g' | { sum=0; while read num; do ((sum+=num)); done; echo $sum; })
APPENDTOFILE_BYTES=$(grep -E "appendToFile.*->" $LOG_FILE | grep -oE '[^ ]+$' | sed -e 's/\.//g' | { sum=0; while read num; do ((sum+=num)); done; echo $sum; })
WRITE_BYTES=$((APPENDTOFILE_BYTES+WRITEFILE_BYTES))
echo -e "writeFile operations: ${WRITEFILE}."
echo -e "writeFile: ${WRITEFILE_BYTES} bytes."
echo -e "appendToFile operations: ${APPENDTOFILE}."
echo -e "append size: ${APPENDTOFILE_BYTES} bytes."
echo -e "total writing operations size: ${WRITE_BYTES} bytes."

# making sure we are not dividing by 0
if [ ${WRITEFILE} -gt 0 ]; then
	MEAN_WRITEFILE=$(echo "${WRITEFILE_BYTES} / ${WRITEFILE}" | bc -l)
	echo -e "writeFile mean: ${MEAN_WRITEFILE} bytes."
fi
if [ ${APPENDTOFILE} -gt 0 ]; then
	MEAN_APPENDTOFILE=$(echo "${APPENDTOFILE_BYTES} / ${APPENDTOFILE}" | bc -l)
	echo -e "appendToFile mean: ${MEAN_APPENDTOFILE} bytes."
fi
if [ ${WRITEFILE} -gt 0 ]; then
	MEAN_WRITE=$(echo "${WRITE_BYTES} / ${WRITES}" | bc -l)
	echo -e "total mean: ${MEAN_WRITE} bytes."
fi


echo -e "-${BOLD}LOCKING OPERATIONS${RESET}-"
# count lines where the regex appears
LOCKFILE=$(grep " lockFile" -c $LOG_FILE)
UNLOCKFILE=$(grep "unlockFile" -c $LOG_FILE)
OPENFILECREATELOCK=$(grep -cE "openFile.* 3 : " $LOG_FILE)
OPENFILELOCK=$(grep -cE "openFile.* 2 : " $LOG_FILE)
OPENLOCK=$((OPENFILECREATELOCK+OPENFILELOCK))
CLOSEFILE=$(grep "closeFile" -c $LOG_FILE)
echo -e "lockFile operations: ${LOCKFILE}."
echo -e "unlockFile operations: ${UNLOCKFILE}."
echo -e "open and lock operations: ${OPENLOCK}."
echo -e "closeFile operations: ${CLOSEFILE}."


echo -e "-${BOLD}CACHING${RESET}-"
# log files have the number of victims inside square brackets
VICTIMS=$(grep -E "Victims : [1-9+]" $LOG_FILE | grep -oE '[^ ]+$' | sed -e 's/\.//g' | wc -l)
MAXSIZE_BYTES=$(grep "Maximum size" $LOG_FILE | grep -oE '[^ ]+$' | sed -e 's/\.//g')
MAXSIZE_MBYTES=$(echo "${MAXSIZE_BYTES} * 0.000001" | bc -l)
MAXFILES=$(grep "Maximum file" $LOG_FILE | grep -oE '[^ ]+$' | sed -e 's/\.//g')
echo -e "Replacement algorithms was executed: ${VICTIMS} time(s)."
echo -e "Maximum size reached by storage: ${MAXSIZE_MBYTES}MBytes."
echo -e "Maximum files reached by storage: ${MAXFILES}."

echo -e "-${BOLD}REQUESTS HANDLED PER WORKER${RESET}-"
# log files have worker ids in curly brackets, counting requests per worker
REQUESTS=$(grep -oP '\[.*?\]' $LOG_FILE  | sort | uniq -c | sed -e 's/[ \t]*//' | sed 's/[][]//g')
while IFS= read -r line; do
	array=($line)
	echo -e "worker with ID number ${array[1]} has handled ${array[0]} requests."
done <<< "$REQUESTS"

echo -e "-${BOLD}ONLINE CLIENTS${RESET}-"
MAXCLIENTS=$(grep "Current online clients : " $LOG_FILE | grep -oE '[^ ]+$' | sed -e 's/\.//g' | sort -g -r | head -1)
echo -e "Maximum number of online clients: ${MAXCLIENTS}."

