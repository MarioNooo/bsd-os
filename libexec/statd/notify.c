#include <rpc/rpc.h>

#include <sm_inter.h>

#include <stdio.h>
#include <string.h>

int
main(int argc, char *argv[])
{

	int status;
	char dummy;
	stat_chge arg;
	struct mon mon;
	struct sm_stat_res stat_res;
	struct sm_stat stat;
	char cmd;

	arg.mon_name = "vapor-machen";
	arg.state = 1;
	
	if (argc != 2)
		return (-1);
	
	/* some common stuff */
	mon.mon_id.mon_name = "forge.bsdi.com";
	memset((char *)mon.priv, 1, sizeof (mon.priv));
	mon.mon_id.my_id.my_name = "forge.bsdi.com";
	mon.mon_id.my_id.my_prog = SM_PROG;
	mon.mon_id.my_id.my_vers = SM_VERS;
	mon.mon_id.my_id.my_proc = SM_MON;

	memset((char *)&stat_res, 0, sizeof (stat_res));

	cmd = *argv[1];
	switch (cmd) {

	case 'n' :
		status = callrpc("labrat", SM_PROG, SM_VERS, 
			SM_NOTIFY, xdr_status, &arg, xdr_void, &dummy);
		break;

	case 'm' :
		status = callrpc("labrat", SM_PROG, SM_VERS, SM_MON,
			(xdrproc_t) xdr_mon, (caddr_t) &mon,
			(xdrproc_t) xdr_sm_stat_res, (caddr_t) &stat_res);
		break;
	case 'u' :
		status = callrpc("labrat", SM_PROG, SM_VERS, SM_UNMON,
			(xdrproc_t) xdr_mon, (caddr_t) &mon,
			(xdrproc_t) xdr_sm_stat, (caddr_t) &stat);
		break;
	case 'a' :
		status = callrpc("labrat", SM_PROG, SM_VERS, SM_UNMON_ALL,
			(xdrproc_t) xdr_my_id, (caddr_t) &mon.mon_id.my_id,
			(xdrproc_t) xdr_sm_stat_res, (caddr_t) &stat_res);
		break;

	default:

		return (-1);

	}

	printf("status = %d\n", status);
	return(0);
}
