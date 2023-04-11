import tftpy


class TFTPTransport:
    def __init__(self, ip_addr, port):
        self.ip_addr = ip_addr
        self.port = port
        self.client = tftpy.TftpClient(ip_addr, port)

    def upload(self, local_file, remote_file):
        self.client.upload(remote_file, local_file)

    def download(self, local_file, remote_file):
        self.client.download(remote_file, local_file)
