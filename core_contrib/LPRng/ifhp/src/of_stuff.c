	} else if( OF_Mode ){
		int fd, banner_done;
		logmsg( "Process_job: starting OF mode passthrough" );
		banner_done = 0;
		while( (fd = Process_OF_mode( Job_timeout, &banner_done)) >= 0 ){
			if( Status_fd > 0 ){
				close( Status_fd );
				Status_fd = -2;
			}
			Set_nonblock_io(1);
			kill(getpid(), SIGSTOP);
			logmsg( "Process_OF_mode: active again");
			if( Appsocket ){
				Open_device( Device );
			}
			Set_nonblock_io(1);
			if( Status ){
				Set_nonblock_io(1);
			}
		}
		logmsg( "Process_job: ending OF mode passthrough" );
