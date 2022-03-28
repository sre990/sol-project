#!/bin/bash

if [ $# -eq 0 ]; then
	echo "Fatal error: please specify a log file path"
	exit 1
fi

BOLD="\033[4m"
RESET="\033[0m"
LOG_FILE=$1


echo -e "-${BOLD}READING OPERATIONS${RESET}-"
# get regular expressions
READFILE=$(grep "readFile" -c $LOG_FILE)
READNFILES=$(grep "readNFiles" -c $LOG_FILE)
# get bytes read
READFILE_BYTES=$(grep -E "readFile.*. Bytes:" $LOG_FILE | grep -oE '[^ ]+$' | sed -e 's/\.//g' | { sum=0; while read num; do ((sum+=num)); done; echo $sum; })
READNFILES_BYTES=$(grep -E "readNFiles.*. Bytes:" $LOG_FILE | grep -oE '[^ ]+$' | sed -e 's/\.//g' | { sum=0; while read num; do ((sum+=num)); done; echo $sum; })
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
	MEAN_READFILE=$(echo "scale=5;${READFILE_BYTES} / ${READFILE}" | bc -l)
	echo -e "readFile mean: ${MEAN_READFILE} bytes."
fi
if [ ${READNFILES} -gt 0 ]; then
	MEAN_READNFILES=$(echo "scale=5;${READNFILES_BYTES} / ${READNFILES}" | bc -l)
	echo -e "readNFiles mean: ${MEAN_READNFILES} bytes."
fi
if [ ${READS} -gt 0 ]; then
	MEAN_READS=$(echo "scale=5;${READ_BYTES} / ${READS}" | bc -l)
	echo -e "total mean: ${MEAN_READS} bytes."
fi


echo -e "-${BOLD}WRITING OPERATIONS${RESET}-"
# get regular expressions
WRITEFILE=$(grep "writeFile" -c $LOG_FILE)
APPENDTOFILE=$(grep "appendToFile" -c $LOG_FILE)
WRITES=$((WRITEFILE+APPENDTOFILE))
# get bytes written
WRITEFILE_BYTES=$(grep -E "writeFile.*. Bytes:" $LOG_FILE | grep -oE '[^ ]+$' | sed -e 's/\.//g' | { sum=0; while read num; do ((sum+=num)); done; echo $sum; })
APPENDTOFILE_BYTES=$(grep -E "appendToFile.*. Bytes:" $LOG_FILE | grep -oE '[^ ]+$' | sed -e 's/\.//g' | { sum=0; while read num; do ((sum+=num)); done; echo $sum; })
WRITE_BYTES=$((APPENDTOFILE_BYTES+WRITEFILE_BYTES))
echo -e "writeFile operations: ${WRITEFILE}."
echo -e "writeFile: ${WRITEFILE_BYTES} bytes."
echo -e "appendToFile operations: ${APPENDTOFILE}."
echo -e "append size: ${APPENDTOFILE_BYTES} bytes."
echo -e "total number of writes: ${WRITES}."
echo -e "total writing operations size: ${WRITE_BYTES} bytes."

# making sure we are not dividing by 0
if [ ${WRITEFILE} -gt 0 ]; then
	MEAN_WRITEFILE=$(echo "scale=5;${WRITEFILE_BYTES} / ${WRITEFILE}" | bc -l)
	echo -e "writeFile mean: ${MEAN_WRITEFILE} bytes."
fi
if [ ${APPENDTOFILE} -gt 0 ]; then
	MEAN_APPENDTOFILE=$(echo "scale=5;${APPENDTOFILE_BYTES} / ${APPENDTOFILE}" | bc -l)
	echo -e "appendToFile mean: ${MEAN_APPENDTOFILE} bytes."
fi
if [ ${WRITEFILE} -gt 0 ]; then
	MEAN_WRITE=$(echo "scale=5;${WRITE_BYTES} / ${WRITES}" | bc -l)
	echo -e "total mean: ${MEAN_WRITE} bytes."
fi


echo -e "-${BOLD}LOCKING OPERATIONS${RESET}-"
LOCKFILE=$(grep " lockFile" -c $LOG_FILE)
UNLOCKFILE=$(grep "unlockFile" -c $LOG_FILE)
OPENFILECREATELOCK=$(grep -cE "openFile.* 3 : " $LOG_FILE)
OPENFILELOCK=$(grep -cE "openFile.* 2 : " $LOG_FILE)
OPENLOCK=$((OPENFILECREATELOCK+OPENFILELOCK))
echo -e "lockFile operations: ${LOCKFILE}."
echo -e "unlockFile operations: ${UNLOCKFILE}."
echo -e "open and lock operations: ${OPENLOCK}."

echo -e "-${BOLD}MISC. OPERATIONS${RESET}-"
REMOVEFILE=$(grep "removeFile" -c $LOG_FILE)
OPENFILE=$(grep -cE "openFile.* 0 : " $LOG_FILE)
OPENS=$((OPENFILE+OPENLOCK))
CLOSEFILE=$(grep "closeFile" -c $LOG_FILE)
echo -e "openFile operations: ${OPENFILE}."
echo -e "total number of opens: ${OPENS}."
echo -e "closeFile operations: ${CLOSEFILE}."
echo -e "removeFile operations: ${REMOVEFILE}."


echo -e "-${BOLD}CACHING${RESET}-"
# get evicted files number
EVICTED=$(grep -E "Evicted: [1-9+]" $LOG_FILE | grep -oE '[^ ]+$' | sed -e 's/\.//g' | wc -l)
MAXSIZE_BYTES=$(grep "Max size" $LOG_FILE | grep -oE '[^ ]+$' | sed -e 's/\.//g')
MAXSIZE_MBYTES=$(echo "${MAXSIZE_BYTES} * 0.000001" | bc -l)
MAXFILES=$(grep "Max number" $LOG_FILE | grep -oE '[^ ]+$' | sed -e 's/\.//g')
SUCCESS=$(grep " : 0\." -c $LOG_FILE)
FAILURE=$(grep " : 1\." -c $LOG_FILE)
echo -e "Replacement algorithm was executed: ${EVICTED} time(s)."
echo -e "Max size reached by the file storage cache: ${MAXSIZE_MBYTES}MBytes."
echo -e "Max number of files stored inside the server: ${MAXFILES}."
echo -e "Number of succesful operations: ${SUCCESS}"
echo -e "Number of failed operations: ${FAILURE}"

echo -e "-${BOLD}REQUESTS HANDLED PER WORKER${RESET}-"
# worker ids are inside []
REQUESTS=$(grep -oP '\[.*?\]' $LOG_FILE  | sort | uniq -c | sed -e 's/[ \t]*//' | sed 's/[][]//g')
while IFS= read -r line; do
	array=($line)
	echo -e "[ID:${array[1]}] Number of requests handled by worker: ${array[0]}."
done <<< "$REQUESTS"

echo -e "-${BOLD}ONLINE CLIENTS${RESET}-"
MAXCLIENTS=$(grep "Clients online now: " $LOG_FILE | grep -oE '[^ ]+$' | sed -e 's/\.//g' | sort -g -r | head -1)
echo -e "Max number of clients online reached: ${MAXCLIENTS}."

