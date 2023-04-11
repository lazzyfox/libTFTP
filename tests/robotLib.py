import tftpy
from ROBOTFTPTestLib import TFTPTransport



def main():
    transport = TFTPTransport('192.168.1.3', 5001)
    transport.upload(test, test)
