//
//  main.c
//  DeepBeliefSDK
//
//  Created by Peter Warden on 4/28/14.
//  Copyright (c) 2014 Jetpac, Inc. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <libjpcnn.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <string.h>

#define WAIT_TIME 2
#define LONG_WAIT_TIME 1500

#define RMSE_THRESHOLD	1800

float* predictions;
int predictionsLength;
char** predictionsLabels;
int predictionsLabelsLength;
const char * networkFileName;
int layer;
const char * predictorFileName;
const char * imageFileName;


void * networkHandle;
void * predictor;

int release=0;
int exit_now=0;
sem_t stopped;
sem_t running;


void unload_module(char * s) {
        pid_t pid=fork();
        if (pid==0) {
                //child
                char * args[] = { "/usr/bin/sudo","/sbin/rmmod", "-f", s, NULL };
                int r = execv(args[0],args);
                fprintf(stderr,"SHOULD NEVER REACH HERE %d\n",r);
        }
        //master
        waitpid(pid,NULL,0);
}

void load_module(char *s ) {
        pid_t pid=fork();
        if (pid==0) {
                //child
                char * args[] = { "/usr/bin/sudo", "/sbin/modprobe", s, NULL };
                int r = execv(args[0],args);
                fprintf(stderr,"SHOULD NEVER REACH HERE %d\n",r);
        }
        //master
        waitpid(pid,NULL, 0);
}

void reload_uvc() {
        fprintf(stderr,"remogin\n");
        unload_module("uvcvideo");
        unload_module("videobuf2_core");
        unload_module("videobuf2_vmalloc");
        unload_module("videodev");
        load_module("videodev");
        load_module("videobuf2_core");
        load_module("videobuf2_vmalloc");
        load_module("uvcvideo");
        fprintf(stderr,"Loaded UVC\n");
        return;
}

float rmse_pictures(char * fn1,char * fn2) {
	//fswebcam here
	int pipefd[2];
	if (pipe(pipefd)==-1) {
		fprintf(stderr,"ERROR IN PIPE CREATE\n");
		exit(1);
	}


	pid_t pid=fork();
	if (pid==0) {
		close(pipefd[0]);
		//child
		dup2(pipefd[1],2);
		char * args[] = { "/usr/bin/compare", "-fuzz",  "5", "-metric",  "RMSE" , fn1,  fn2,  "/dev/null", NULL};
		int r = execv(args[0],args);
		fprintf(stderr,"SHOULD NEVER REACH HERE %d\n",r);
	}
	close(pipefd[1]);
	//get the RMSE
	char buffer[1024];
	int r = read(pipefd[0], buffer, 1024);
	float rmse=0.0;
	if (r<=0) {
		fprintf(stderr,"ERROR IN READING\n");
	} else {
		sscanf(buffer, "%f",&rmse);
	}
	//master
	waitpid(pid,NULL,0);	
	close(pipefd[0]);
	return rmse;
}
int take_picture(char * fn) {
	//fswebcam here
	unlink(fn); //remove the file if it exists
	int i=0;
	while ( access( fn, F_OK ) == -1 ) {
		if (i>0) {
			sleep(1); //give it a little rest if we cant get picture
			if (i%4==0) {
				reload_uvc();			
			}
		} 
		if (release==1) {
			break;
		}
		pid_t pid=fork();
		if (pid==0) {
			//child
			//TODO make sure acquired image file
			char * args[] = { "/usr/bin/fswebcam","-r","640x480","--skip","5",
				" --no-underlay","--no-info","--no-banner","--no-timestamp","--quiet",fn, NULL };
			int r = execv(args[0],args);
			fprintf(stderr,"SHOULD NEVER REACH HERE %d\n",r);
		}
		//master
		waitpid(pid,NULL,0);
		i++;	
	}
	return 0;
}


void busy_wait(int s) {
	while (s>0) {
		sleep(1);
		if (release==1) {
			return;
		}
		s--;
	}
}



int check_for_dog(char * fn ) {
	if (release==1) {
		return 0;
	}	
	void * imageHandle = jpcnn_create_image_buffer_from_file(fn);
	if (imageHandle == NULL) {
		fprintf(stderr, "DeepBeliefSDK: Couldn't load image file '%s'\n", fn);
		return 0;
	}

	//next classify
	if (release==1) {
		return 0;
		jpcnn_destroy_image_buffer(imageHandle);
	}	

	jpcnn_classify_image(networkHandle, imageHandle, 0, layer, &predictions, &predictionsLength, &predictionsLabels, &predictionsLabelsLength);
	jpcnn_destroy_image_buffer(imageHandle);


	//next predict
	if (release==1) {
		return 0;
	}	
	
	float pred = jpcnn_predict(predictor, predictions, predictionsLength);
	fprintf(stdout,"Atos probability %f\n",pred);

	//next predict
	if (release==1) {
		return 0;
	}	

	//next send out the image if it passes
	if (pred>0.11) {
		char pred_s[1024];
		sprintf(pred_s,"%0.4f", pred);
		int pid=fork();
		if (pid==0) {
			//child
			char * args[] = { "/bin/bash","./send_atos.sh",fn, pred_s, NULL };
			int r = execv(args[0],args);
			fprintf(stderr,"SHOULD NEVER REACH HERE %d\n",r);
		}
		return 1;
	}
	return 0;
}





void * analyze() {
	char currentImageFileName[1024];
	char previousImageFileName[1024];
	sprintf(currentImageFileName,"%s_current.jpg",imageFileName);
	sprintf(previousImageFileName,"%s_previous.jpg",imageFileName);


	fprintf(stderr,"Loading network %s\n",networkFileName);
	networkHandle = jpcnn_create_network(networkFileName);
	if (networkHandle == NULL) {
		fprintf(stderr, "DeepBeliefSDK: Couldn't load network file '%s'\n", networkFileName);
		return;
	}
	fprintf(stderr,"Loading predictor %s\n",predictorFileName);
	predictor = jpcnn_load_predictor(predictorFileName);
	if (predictor==NULL) {
		fprintf(stderr,"Failed to load predictor\n");
		return;
	}

	//main loop
	while (1>0) {
		sem_wait(&running);
		
		
		//detect-and-work loop
		while (1>0) {
			
			if (release==1) {
				break;
			}
	
			//first picture
			take_picture(currentImageFileName);

			busy_wait(WAIT_TIME);
			if (release==1) {
				break;
			}	
			
			//motion loop
			int diff=0;
			int motion=3;
			while (diff<2500) {
				if (release==1) {
					break;
				}	
				//move current to previous
				rename(currentImageFileName,previousImageFileName);
		
				//get current picture
				take_picture(currentImageFileName);
				//if no motion the wait a bit
				if (motion<=0) {
					busy_wait(WAIT_TIME);
				} else {
					motion--;
				}

				if (release==1) {
					break;
				}
			
				//compare
				float rmse = rmse_pictures(currentImageFileName,previousImageFileName);
				fprintf(stderr,"RMSE is %f\n",rmse);

				//check for motion
				if (rmse>RMSE_THRESHOLD) {
					fprintf(stderr,"passed threshold moving on to detector...\n");;
					motion=10;
					int check = check_for_dog(currentImageFileName);	
					if (check==1) {
						busy_wait(LONG_WAIT_TIME);
					}
				}
			}
		}
		if (exit_now==1) {
			break;
		}
	  	sem_post(&stopped);

  	}

	fprintf(stderr, "Cleaning up predictor...\n");
	jpcnn_destroy_predictor(predictor);
	fprintf(stderr,"Cleaning up network...\n");
	jpcnn_destroy_network(networkHandle);
	sem_post(&stopped);

}

int main(int argc, const char * argv[]) {


  


  if (argc!=5) {
	fprintf(stderr,"%s network_filename layer svm_filename img_filename\n",argv[0]);
	exit(1);
  }

  networkFileName=argv[1];
  layer = atoi(argv[2]);
  predictorFileName=argv[3];
  imageFileName=argv[4];


        sem_init(&stopped,0,0);
        sem_init(&running,0,0);

        pthread_t analyze_thread;
        int  iret1;


	release=1;

        //start the tcp_client
        iret1 = pthread_create( &analyze_thread, NULL, analyze, NULL);
        if(iret1) {
                fprintf(stderr,"Error - pthread_create() return code: %d\n",iret1);
                exit(EXIT_FAILURE);
        }


	//monitor STDIN/ STDOUT
	char buffer[1024];
	while (1>0) {
		fgets(buffer, 1024, stdin);
		if (strlen(buffer)>0 ) {
			buffer[strlen(buffer)-1]='\0';
		}
		if (strcmp(buffer,"STOP")==0) {
			if (release==1) {
				//hmmmm	
			} else {
				release=1;
				sem_wait(&stopped); //wait for other guy to stop
				//when here other guy is stopped
				fprintf(stdout,"STOPPED\n");	
			}
		} else if (strcmp(buffer,"GO")==0) {
			if (release==0) {

			} else {
				release=0;	
				sem_post(&running);
			}	
		} else if (strcmp(buffer,"EXIT")==0) {
			exit_now=1;
			if (release==1) {
				//hmmmm	
			} else {
				release=1;
				sem_wait(&stopped); //wait for other guy to stop
				//when here other guy is stopped
				fprintf(stdout,"STOPPED\n");	
			}
			return 0;
		}
	}	

  return 0;
}
