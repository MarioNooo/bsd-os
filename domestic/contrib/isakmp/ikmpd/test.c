#include <stdio.h>

int main (int argc, char **argv)
{
    FILE *fp;
    char prot[4], trans[10], additional[10], buffer[80];


    fp = fopen("ipsec_def", "r");
    if(fp == NULL){
	fprintf(stderr,"%s: Unable to open definition file\n", argv[0]);
	exit(1);
    }
    fscanf(fp, "%s", buffer);
    while(!feof(fp)){
	if(buffer[0] == '#'){
	    fscanf(fp, "%s", buffer);
	    continue;
	}
	sscanf(buffer, "%s %s %s", prot, trans, additional);
	if(bcmp(prot, "ah", 2) == 0){
	    printf("authentication header: "); fflush(stdout);
	    if(bcmp(trans, "md5", 3) == 0){
		printf("md5\n");
	    } else if(bcmp(trans, "sha", 3) == 0){
		printf("sha\n");
	    } else if(bcmp(trans, "hmac-sha", 8) == 0){
		printf("sha in hmac\n");
	    } else if(bcmp(trans, "hmac-md5", 8) == 0){
		printf("md5 in hmac\n");
	    }
	} else if(bcmp(prot, "esp") == 0){ 
	    printf("encapsuating security protocol: "); fflush(stdout);
	    if(bcmp(trans, "des-cbc", 7) == 0){
		printf("DES in CBC mode\n");
	    }
	} else if(bcmp(prot, "mm", 2) == 0){
	    printf("desired exchange is Main Mode\n");
	} else if(bcmp(prot, "ag", 2) == 0){
	    printf("desired exchange is Aggressive Mode\n");
	}
	fscanf(fp, "%s", buffer);
    }
    printf("end of file\n");
}
