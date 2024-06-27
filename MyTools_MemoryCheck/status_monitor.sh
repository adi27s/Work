#!/bin/sh

# Function to monitor memory parameters for a process named 'person_det'
monitor_memory() {
    # Clear the screen
    clear

    # Find the PID of the process named 'person_det'
    PID=$(pgrep person_det)

    # Check if the PID was found
    if [ -z "$PID" ]; then
        echo "Process 'person_det' not found."
        exit 1
    fi

    # Initialize variables for current and peak values
    VmPeak=0
    VmSize=0
    VmHWM=0
    VmRSS=0
    VmData=0
    VmStk=0
    VmExe=0
    VmLib=0
    RssAnon=0
    RssFile=0
    RssShmem=0

    # Read memory parameters from the status file
    while IFS= read -r line; do
        case "$line" in
            VmPeak:*) VmPeak=$(echo "$line" | awk '{print $2}') ;;
            VmSize:*) VmSize=$(echo "$line" | awk '{print $2}') ;;
            VmHWM:*) VmHWM=$(echo "$line" | awk '{print $2}') ;;
            VmRSS:*) VmRSS=$(echo "$line" | awk '{print $2}') ;;
            VmData:*) VmData=$(echo "$line" | awk '{print $2}') ;;
            VmStk:*) VmStk=$(echo "$line" | awk '{print $2}') ;;
            VmExe:*) VmExe=$(echo "$line" | awk '{print $2}') ;;
            VmLib:*) VmLib=$(echo "$line" | awk '{print $2}') ;;
            RssAnon:*) RssAnon=$(echo "$line" | awk '{print $2}') ;;
            RssFile:*) RssFile=$(echo "$line" | awk '{print $2}') ;;
            RssShmem:*) RssShmem=$(echo "$line" | awk '{print $2}') ;;
        esac
    done < "/proc/$PID/status"

    # Convert sizes from kB to MB for readability
    VmPeak_MB=$((VmPeak / 1024))
    VmSize_MB=$((VmSize / 1024))
    VmHWM_MB=$((VmHWM / 1024))
    VmRSS_MB=$((VmRSS / 1024))
    VmData_MB=$((VmData / 1024))
    VmStk_MB=$((VmStk / 1024))
    VmExe_MB=$((VmExe / 1024))
    VmLib_MB=$((VmLib / 1024))
    RssAnon_MB=$((RssAnon / 1024))
    RssFile_MB=$((RssFile / 1024))
    RssShmem_MB=$((RssShmem / 1024))

    # Calculate and store peak values in MB
    if [ $VmData_MB -gt $VmPeakData_MB ]; then
        VmPeakData_MB=$VmData_MB
    fi
    if [ $VmStk_MB -gt $VmPeakStk_MB ]; then
        VmPeakStk_MB=$VmStk_MB
    fi
    if [ $VmExe_MB -gt $VmPeakExe_MB ]; then
        VmPeakExe_MB=$VmExe_MB
    fi
    if [ $VmLib_MB -gt $VmPeakLib_MB ]; then
        VmPeakLib_MB=$VmLib_MB
    fi
    if [ $RssAnon_MB -gt $RssAnonPeak_MB ]; then
        RssAnonPeak_MB=$RssAnon_MB
    fi
    if [ $RssFile_MB -gt $RssFilePeak_MB ]; then
        RssFilePeak_MB=$RssFile_MB
    fi
    if [ $RssShmem_MB -gt $RssShmemPeak_MB ]; then
        RssShmemPeak_MB=$RssShmem_MB
    fi

    # Display memory parameters with peak calculations
    echo "Memory Parameters for PID $PID (process 'person_det'):"
    echo "------------------------------------------------------"
    echo "VmSize:    $VmPeak kB    ($VmPeak_MB MB)   - Peak virtual memory used"
    echo "VmSize:    $VmSize kB    ($VmSize_MB MB)   - Current virtual memory size"
    echo "VmHWM:     $VmHWM kB     ($VmHWM_MB MB)    - Peak physical memory used"
    echo "VmRSS:     $VmRSS kB     ($VmRSS_MB MB)    - Current physical memory used"
    echo "------------------------------------------------------"
    echo "Memory Segments:"
    echo "----------------"
    echo -n "VmData:    $VmData kB    ($VmData_MB MB)   - Data segment"
    echo "               Peak: $VmPeakData kB   ($VmPeakData_MB MB)"
    echo -n "VmStk:     $VmStk kB     ($VmStk_MB MB)    - Stack segment"
    echo "               Peak: $VmPeakStk kB    ($VmPeakStk_MB MB)"
    echo -n "VmExe:     $VmExe kB     ($VmExe_MB MB)    - Code segment"
    echo "                Peak: $VmPeakExe kB    ($VmPeakExe_MB MB)"
    echo -n "VmLib:     $VmLib kB     ($VmLib_MB MB)    - Shared library segment"
    echo "   Peak: $VmPeakLib kB    ($VmPeakLib_MB MB)"
    echo "------------------------------------------------------"
    echo "Memory Types:"
    echo "-------------"
    echo -n "RssAnon:   $RssAnon kB   ($RssAnon_MB MB)  - Anonymous memory (heap)"
    echo "  Peak: $RssAnonPeak kB   ($RssAnonPeak_MB MB)"
    echo -n "RssFile:   $RssFile kB   ($RssFile_MB MB)  - File-backed memory"
    echo "       Peak: $RssFilePeak kB   ($RssFilePeak_MB MB)"
    echo -n "RssShmem:  $RssShmem kB  ($RssShmem_MB MB) - Shared memory"
    echo "                 Peak: $RssShmemPeak kB  ($RssShmemPeak_MB MB)"
    echo "------------------------------------------------------"
}

# Initialize peak variables to zero
VmPeakData_MB=0
VmPeakStk_MB=0
VmPeakExe_MB=0
VmPeakLib_MB=0
RssAnonPeak_MB=0
RssFilePeak_MB=0
RssShmemPeak_MB=0

# Continuously monitor memory parameters every 1 second
while true; do
    monitor_memory
    sleep 1
done

