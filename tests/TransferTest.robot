*** Settings ***
Documentation     Example using the space separated format.
Library           OperatingSystem
Library           ROBOTFTPTestLib/TFTPTestLib.py

*** Variables ***
${MESSAGE}        Hello, world!


*** Test Cases ***
My Test
    [Documentation]    Example test.
    Log    ${MESSAGE}
    My Keyword    ${CURDIR}


*** Keywords ***
My Keyword
    [Arguments]    ${path}
    Directory Should Exist    ${path}