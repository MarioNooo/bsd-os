/* BSDI compare.i,v 2.2 1996/02/29 18:14:49 bostic Exp */

DECLARE (total_time)

{
  SETUP;
  return comp_add (L (ac_utime), L (ac_stime)) -
	 comp_add (R (ac_utime), R (ac_stime));
}

DECLARE (average_time)

{
  COMP_T l;
  COMP_T r;

  SETUP;
  l = (comp_conv (L (ac_utime)) + comp_conv (L (ac_stime))) /
    L (ac_count);
  r = (comp_conv (R (ac_utime)) + comp_conv (R (ac_stime))) /
    R (ac_count);
  return l - r;
}

DECLARE (average_disk)

{
  SETUP;
  return comp_conv (L (ac_io)) / L (ac_count) -
	 comp_conv (R (ac_io)) / R (ac_count);
}

DECLARE (total_disk)

{
  SETUP;
  return L (ac_io) - R (ac_io);
}

DECLARE (average_memory)

{
  SETUP;
  return L (ac_mem) - R (ac_mem);
}

DECLARE (kcore_secs)

{
  COMP_T l;
  COMP_T r;

  SETUP;
  l = (comp_conv (L (ac_utime)) + comp_conv (L (ac_stime))) *
    L (ac_mem);
  r = (comp_conv (R (ac_utime)) + comp_conv (R (ac_stime))) *
    R (ac_mem);

  return l - r;
}

DECLARE (call_count)

{
  SETUP;
  return L (ac_count) - R (ac_count);
}
