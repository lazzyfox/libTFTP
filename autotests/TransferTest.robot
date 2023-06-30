*** Settings ***
Documentation      Lib TFTP version 0.0.2 test 
Library            OperatingSystem
Library		   Process
Library            ./TFTPTransport.py     ${ServerIP_v4_Val}     ${PortID_Val}
Library            ./libRunStop.py      ${TestWorkDir}    ${TestLogDir}
Library            ./IPAddrParam.py
#Test Setup         libRunStop.startProc    ${RunArg}
#Test Teardown     Close Application

*** Variables ***
${OUTPUT DIR}          ${CURDIR}${/}TestLog
${MESSAGE}              Test sute ver 0.0.2
${UploadMsg}            Upload test file test
${DownloadMsg}          Download test file test
${TestComplet}          Test complete
${ProcRunCheckTrue}     Process running
${ProcRunCheckSleep}    Process sleping
${ProcRunCheckFalse}    Process running check failed
${SrvStop}              Stopping TFTP Server
${PortID_Key}          -p
${PortID_Val}=          5001
${ServerIP_Key}        -a
${ServerIP_V4_Val}=        127.0.0.1
${ServerIP_V6_Val}=        0000:0000:0000:0000:0000:0000:0000:0001
${UplFileName_Key}     -u
${UplFileName_Val}      './test.txt'
${DownFileName_Key}     -d
${DownFileName_Val}     test.txt
${LocalDirPath_Key}     -l
${LocalDirPath_Val}     ${TEMPDIR}
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
${TestWorkDir}          ${TEMPDIR}${/}tftp_test${/}work
${TestLogDir}           ${TEMPDIR}${/}tftp_test${/}log
${TestDownLoadDir}      ${TEMPDIR}${/}tftp_test${/}download
${TestDataDir}          ${CURDIR}${/}TestData
@{BigDataFiles}         ak.html    ak.txt    ak.pdf
${PidId}=               0
${RunStatus} =          running
${SleepStatus}=         sleeping
${SrvPath}=             ${CURDIR}${/}../build/bin/srv/tftp_srv


*** Test Cases ***
Start Test
    [Documentation]    Run TFTP server.
    Log    ${MESSAGE}
    ${PidId}=    libRunStop.runProc     ${SrvPath}     -p     ${PortID_Val}     -a     '${ServerIP_V4_Val}'    -l    '${TestLogDir}'    -d    '${TestWorkDir}'
    Sleep    3s
    CheckServerStatus
    Log    ${PidId}

Download Python
    [Documentation]    Download test file by Python client.
    Log    ${DownloadMsg}
    CheckServerStatus
    FOR    ${file_name}    IN    @{BigDataFiles}
        ${Ret}    TFTPTransport.flCopy      ${TestDataDir}${/}${file_name}    ${TestWorkDir}${/}${file_name}
        Should Be True    ${Ret}
        ${Ret}    TFTPTransport.downLoad    ${TestDownLoadDir}${/}${file_name}     ${file_name}
        Should Be True    ${Ret}
        ${Ret}    TFTPTransport.flCompare    ${TestDownLoadDir}${/}${file_name}    ${TestDataDir}${/}${file_name}
        TFTPTransport.flRemove    ${TestDownLoadDir}${/}${file_name}
    END
    Log    ${TestComplet}

Upload Python
    [Documentation]    Upload test file by Python client.
    Log    ${UploadMsg}
    CheckServerStatus
    ${Ret}    TFTPTransport.upLoad    '${UplFileName_Val}'    '${UplFileName_Val}''
    Should Be True    ${Ret}

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
SetIPV4
    ${ServerIP_V4_Val}=    IPAddrParam.getAddr('v4')
SetIPV6
    ${ServerIP_V6_Val}=    IPAddrParam.getAddr('v6')