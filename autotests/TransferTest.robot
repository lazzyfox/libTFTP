*** Settings ***
Documentation      Lib TFTP version 0.0.2 test 
Library            OperatingSystem
Library            /home/fox/fast_proj/libTFTP/tests/TFTPTransport.py     ${ServerIP_Val}     ${PortID_Val}
Library            /home/fox/fast_proj/libTFTP/tests/libRunStop.py      ${TestWorkDir}    ${TestLogDir}
#Test Setup         libRunStop.startProc    ${RunArg}
#Test Teardown     Close Application

*** Variables ***
${MESSAGE}             Test sute ver 0.0.1
${UploadMsg}           Upload test file test
${DownloadMsg}         Download test file test
${ProcRunCheckTrue}    Process running
${ProcRunCheckSleep}    Process sleping
${ProcRunCheckFalse}    Process running check failed
${SrvStop}              Stopping TFTP Server
${PortID_Key}          -p
${PortID_Val}=          5001
${ServerIP_Key}        -a
${ServerIP_Val}=        '192.168.1.5'
${UplFileName_Key}     -u
${UplFileName_Val}      './test.txt'
${DownFileName_Key}     -d
${DownFileName_Val}     test.txt
${LocalDirPath_Key}     -l
${LocalDirPath_Val}     /tmp
${LocalFileName_Key}    -f
${LocalFileName_Val}    './test.txt'
${PackSize_Key}         -b
${PackSize_Val}         1024
${TimeOut_Key}          -t
${TimeOut_Val}          3
${TransMode_Key}        -m
${TimeOut_Val_Octet}    octet
${TimeOut_Val_ACII}     ascii
${Quit_Key}             -q
${Help_Key}             -?
${TestWorkDir}     "/tmp/tftp_test"
${TestLogDir}     "/tmp/tftp_test"
${PidId}=     0
${RunStatus} =     running
${SleepStatus}=     sleeping
${SrvPath}=     /home/fox/fast_proj/libTFTP/build/bin/srv/tftp_srv


*** Test Cases ***
Start Test
    [Documentation]    Run TFTP server.
    Log    ${MESSAGE}
    ${PidId}=    libRunStop.runProc     ${SrvPath}     -p     ${PortID_Val}     -a     '${ServerIP_Val}'    -l    '${TestLogDir}'    -d    '${TestWorkDir}'
    CheckServerStatus
    Log    ${PidId}

Download Python
    [Documentation]    Download test file by Python client.
    Log    ${DownloadMsg}
    CheckServerStatus
    TFTPTransport.downLoad    '${DownFileName_Val}'    '${DownFileName_Val}'

Upload Python
    [Documentation]    Upload test file by Python client.
    Log    ${UploadMsg}
    CheckServerStatus
    TFTPTransport.upLoad    '${UplFileName_Val}'    '${UplFileName_Val}''

Stop TFTP server
    [Documentation]    Stopiing TFTP SERVER
    Log    ${SrvStop}
    ${ProcStatus}=    libRunStop.statusProc
    IF     '${ProcStatus}' == '${RunStatus}'
        Log     Stopping server
        libRunStop.stopTest
    ELSE IF    '${ProcStatus}' == '${SleepStatus}'
        Log     Stopping server
        libRunStop.stopTest
    END


*** Keywords ***
CheckServerStatus
    ${ProcStatus}=    libRunStop.statusProc
    #${ProcessPID}=    libRunStop.checkPid

    IF     '${ProcStatus}' == '${RunStatus}'
        Log     ${ProcRunCheckTrue}
    ELSE IF    '${ProcStatus}' == '${SleepStatus}'
        Log     ${ProcRunCheckSleep}
    ELSE
        Log     ${ProcRunCheckFalse}
    END

    Should Be True    '${ProcStatus}' == '${RunStatus}' or '${ProcStatus}' =='${SleepStatus}'
