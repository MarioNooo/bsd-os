#! /bin/sh
:
PATH=/usr/bin:/usr/ucb:xDESTROOTx/bin ; export PATH

netstat -f inet -n | grep ESTAB | itest

