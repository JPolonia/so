switch(cmd) {
  case SCULL_IOCRESET:
    scull_quantum = SCULL_QUANTUM;
    scull_qset = SCULL_QSET;
    break;
  case SCULL_IOCSQUANTUM: /* Set: arg points to the value */
    if (! capable (CAP_SYS_ADMIN))
        return -EPERM;
    retval = __get_user(scull_quantum, (int __user *)arg);
    break;
  case SCULL_IOCTQUANTUM: /* Tell: arg is the value */
    if (! capable (CAP_SYS_ADMIN))
        return -EPERM;
    scull_quantum = arg;
    break;
  case SCULL_IOCGQUANTUM: /* Get: arg is pointer to result */
    retval = __put_user(scull_quantum, (int __user *)arg);
    break;
  case SCULL_IOCQQUANTUM: /* Query: return it (it's positive) */
    return scull_quantum;
  case SCULL_IOCXQUANTUM: /* eXchange: use arg as pointer */
    if (! capable (CAP_SYS_ADMIN))
,ch06.8719  Page 145  Friday, January 21, 2005  10:44 AM
This is the Title of the Book, eMatter Edition
Copyright © 2005 O’Reilly & Associates, Inc. All rights reserved.
146
|
Chapter 6:   Advanced Char Driver Operations
        return -EPERM;
    tmp = scull_quantum;
    retval = __get_user(scull_quantum, (int __user *)arg);
    if (retval = = 0)
        retval = __put_user(tmp, (int __user *)arg);
    break;
  case SCULL_IOCHQUANTUM: /* sHift: like Tell + Query */
    if (! capable (CAP_SYS_ADMIN))
        return -EPERM;
    tmp = scull_quantum;
    scull_quantum = arg;
    return tmp;
  default:  /* redundant, as cmd was checked against MAXNR */
    return -ENOTTY;
}
return retval;