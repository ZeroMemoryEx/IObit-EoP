# Details

* The PoC program exploits the IMFForceDelete driver which exposes an ioctl that allows unprivileged users to delete files/folders. We can turn this into a privilege escalation by using a technique explained by ZDI and Halov, which exploits the MSI rollback mechanism which is designed to maintain system integrity in case of issues. By deleting and recreating it with a weak DACL and fake RBF and RBS, we gain the ability to make arbitrary changes to the system as NT AUTHORITY\SYSTEM.

# VID
https://github.com/user-attachments/assets/4aa22fc3-424e-4e9f-af4d-38989de029f1

# Creadit

* [KLINIX5](https://x.com/KLINIX5) 
* [ZDI](https://www.zerodayinitiative.com/blog/2022/3/16/abusing-arbitrary-file-deletes-to-escalate-privilege-and-other-great-tricks-archive)
* [vx-underground](https://x.com/vxunderground)
* [#ifndef hjonk](https://x.com/_mmpte_software)

