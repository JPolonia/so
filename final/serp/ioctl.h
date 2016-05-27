#include <linux/ioctl.h>

// IOCTL
/*
 * S means "Set" through a ptr,
 * T means "Tell" directly
 * G means "Get" (to a pointed var)
 * Q means "Query", response is on the return value
 * X means "eXchange": G and S atomically
 * H means "sHift": T and Q atomically
 */
#define SERP_IOC_MAGIC    0x81      //random.org
#define SERP_IOCGLCR     _IOR(SERP_IOC_MAGIC, 1, int) // Obter o valor de LCR e baud
#define SERP_IOCSLCR     _IOW(SERP_IOC_MAGIC, 2, int) // Definir o valor de LCR
#define SERP_IOCSBAUD    _IOW(SERP_IOC_MAGIC, 3, int) // Definir o valor da baud rate
#define SERP_IOC_MAXNR    3
