# Description

* The PoC program exploits the IMFForceDelete function, which exposes an ioctl that allows unprivileged users to delete files and folders. We can turn this into a privilege escalation by using a technique explained by ZDI and Halov, which exploits the MSI rollback mechanism which is designed to maintain system integrity in case of issues. By deleting and recreating it with a weak DACL and fake RBF and RBS files we can gain the ability to make arbitrary changes to the system as NT AUTHORITY\SYSTEM
  
# VID

https://github.com/user-attachments/assets/58e343d2-97a4-4ca3-9deb-df911b717a57

# CREDITS

* [Halov](https://x.com/KLINIX5)
* [ZDI](https://www.zerodayinitiative.com/blog/2022/3/16/abusing-arbitrary-file-deletes-to-escalate-privilege-and-other-great-tricks-archive)
* [vx-underground](https://x.com/vxunderground)
* [#ifndef hjonk](https://x.com/_mmpte_software)
  