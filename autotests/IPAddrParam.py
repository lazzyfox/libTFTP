import netifaces

class IPAddrParam:
    def getAddr(self, ver):
        version = netifaces.AF_INET
        if ver in ['v6', 'V6'] :
            version = netifaces.AF_INET6
        address = netifaces.gateways()
        return address[version][0][0]
