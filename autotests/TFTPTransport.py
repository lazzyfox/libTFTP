import tftpy
import os
import filecmp

class TFTPTransport:
    ip_addr = '127.0.0.1'
    port = 5001
    client = tftpy

    def __init__(self, ip_addr, port):
        TFTPTransport.ip_addr = ip_addr
        TFTPTransport.port = port
        TFTPTransport.client = tftpy.TftpClient(ip_addr, port)

    def upLoad(self, local_file, remote_file):
        TFTPTransport.client.upload(remote_file, local_file)

    def downLoad(self, local_file, remote_file):
        TFTPTransport.client.download(remote_file, local_file)
    
    def flSize(self, fl_name):
        file_size = os.path.getsize(fl_name)
        return file_size
    
    def flCompare(self, fl_one, fl_two):
        result = filecmp.cmp(fl_one, fl_two, shallow=False)
        return result
    
    def flRemove(self, fl_name):
        os.remove(fl_name)
