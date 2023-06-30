import tftpy
import os
import filecmp
import shutil

class TFTPTransport:
    ip_addr = '127.0.0.1'
    port = 5001
    client = tftpy
    
    def __init__(self, ip_addr, port):
        TFTPTransport.ip_addr = ip_addr
        TFTPTransport.port = port
        TFTPTransport.client = tftpy.TftpClient(ip_addr, port)

    def upLoad(self, local_file, remote_file):
        ret = bool(1)
        if not os.path.exists(local_file):
            return bool(0)

        try:
            TFTPTransport.client.upload(remote_file, local_file)
        except Exception as e:
            ret = bool(0)
        return ret
    
    def downLoad(self, local_file, remote_file):
        ret = bool(1)
        if os.path.exists(local_file):
            os.remove(local_file)
        try:
            TFTPTransport.client.download(remote_file, local_file)
        except Exception as e:
            ret = bool(0)
        return ret
    
    def flSize(self, fl_name):
        file_size = os.path.getsize(fl_name)
        return file_size
    
    def flCompare(self, fl_one, fl_two):
        ret = bool (1)
        try:
            ret = filecmp.cmp(fl_one, fl_two, shallow=False)
        except Exception as e:
            ret = bool(0)
        return ret
    
    def flRemove(self, fl_name):
        os.remove(fl_name)
    
    def flCopy(self, local_file, remote_file):
        ret = bool(1)
        if not os.path.exists(local_file):
            return bool(0)
        if os.path.exists(remote_file):
            return bool(1)
        try:
            shutil.copyfile(local_file, remote_file)
        except Exception as e:
            ret = bool(0)
        return ret
    
    def flExists(self, fl_name):
        ret = bool(1)
        if not os.path.exists(fl_name):
            ret = bool(0)
        return ret