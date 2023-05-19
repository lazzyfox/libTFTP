import subprocess
import psutil
import docker
import shutil
import os
import sys


class libRunStop:
    pid = 0
    proc = 0
    work_dir = "/tmp/tftp_test"
    log_dir = "/tmp/tftp_test"

    def __init__(self, work_path, log_path):
        libRunStop.work_dir = work_path
        libRunStop.log_dir = log_path

        if not os.path.exists(libRunStop.work_dir):
            os.makedirs(libRunStop.work_dir)
        if not os.path.exists(libRunStop.log_dir):
            os.makedirs(libRunStop.log_dir)

    def startProc(self, args):
        libRunStop.proc = subprocess.Popen([*args], shell=False)
        libRunStop.pid = libRunStop.proc.pid
        return libRunStop.pid

    def runProc(self, *argv):
        list_args = []
        for arg in argv:
            list_args.append(arg)
        libRunStop.proc = subprocess.Popen([*list_args], shell=False)
        libRunStop.pid = libRunStop.proc.pid
        return libRunStop.pid

    def getPid(self):
        return libRunStop.pid

    def checkPid(self):
        return libRunStop.proc.pid

    def getProc(self):
        return libRunStop.proc
    
    def statusProc(self):
        status = psutil.Process(libRunStop.pid)
        return status.status()

    def clearTestDir(self):
        if os.path.exists(libRunStop.work_dir):
            shutil.rmtree(libRunStop.work_dir)
        if os.path.exists(libRunStop.log_dir):
            shutil.rmtree(libRunStop.log_dir)

    def stopTest(self):
        if not libRunStop.proc == 0:
            libRunStop.proc.kill()
            libRunStop.pid = 0
            libRunStop.proc = 0
            
        libRunStop.clearTestDir(self)

