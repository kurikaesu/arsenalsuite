#!/usr/bin/python2.5

import os, sys, pwd

rc_master_server="drops01"
rc_master_port=90003

wsql_master_server="drops01"
wsql_master_port=90001

wpgsql_master_server="om012951"
wpgsql_master_port=1

priorityCfgFile="/drd/software/int/sys/renderops/config_files/shot_prio.ini"
priorityLogFile="prio_log.txt"

shotProjCfgFile="/drd/software/int/sys/renderops/config_files/shot_proj.ini"
shotProjLogFile="proj_log.txt"

serviceTypeCfgFile="/drd/software/int/sys/renderops/config_files/service_types.ini"
serviceCfgFile="/drd/software/int/sys/renderops/config_files/service_cfg.ini"

current_user=pwd.getpwuid(os.getuid()).pw_name

