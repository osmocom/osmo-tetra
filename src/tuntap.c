#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <sys/ioctl.h>

#include <linux/if_tun.h>
#include <linux/if.h>

int tun_alloc(char *dev)
  {
      struct ifreq ifr;
      int fd, err;

      fd = open("/dev/net/tun", O_RDWR);
	if (fd < 0)
		return fd;
        // return tun_alloc_old(dev);

      memset(&ifr, 0, sizeof(ifr));

      /* Flags: IFF_TUN   - TUN device (no Ethernet headers) 
       *        IFF_TAP   - TAP device  
       *
       *        IFF_NO_PI - Do not provide packet information  
       */ 
      ifr.ifr_flags = IFF_TUN|IFF_NO_PI; 
      if( *dev )
         strncpy(ifr.ifr_name, dev, IFNAMSIZ);

      if( (err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0 ){
         close(fd);
         return err;
      }
#if 0
      strcpy(dev, ifr.ifr_name);
#endif
      return fd;
  }              
